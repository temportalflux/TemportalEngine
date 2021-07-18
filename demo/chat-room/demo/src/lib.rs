use engine::{network, utility::VoidResult, Application};
pub use temportal_engine as engine;

#[path = "packet/mod.rs"]
pub mod packet;

#[path = "ui/mod.rs"]
pub mod ui;

pub struct ChatRoom();
impl Application for ChatRoom {
	fn name() -> &'static str {
		std::env!("CARGO_PKG_NAME")
	}
	fn version() -> semver::Version {
		semver::Version::parse(std::env!("CARGO_PKG_VERSION")).unwrap()
	}
}

pub fn run() -> VoidResult {
	engine::logging::init(ChatRoom::name(), None)?;
	packet::register_types();
	let mut engine = engine::Engine::new()?;
	engine.scan_paks()?;

	let app = std::sync::Arc::new(std::sync::RwLock::new(App()));
	if let Ok(mut network) = network::Network::write() {
		network.add_observer(std::sync::Arc::downgrade(&app));
	}

	let network_port = std::env::args()
		.find_map(|arg| {
			arg.strip_prefix("-port=")
				.map(|s| s.parse::<u16>().ok())
				.flatten()
		})
		.unwrap_or(25565);
	if let Ok(mut network) = network::Network::write() {
		network.start(if std::env::args().any(|arg| arg == "-server") {
			network::Config {
				mode: network::Kind::Server.into(),
				port: network_port,
			}
		} else {
			network::Config {
				mode: network::Kind::Client.into(),
				port: network_port,
			}
		})?;
	}

	if network::mode().contains(network::Kind::Client) {
		engine::window::Window::builder()
			.with_title("Chat Room")
			.with_size(1280.0, 720.0)
			.with_resizable(true)
			.with_application::<ChatRoom>()
			.with_clear_color([0.08, 0.08, 0.08, 1.0].into())
			.build(&mut engine)?;

		engine::ui::System::new(engine.render_chain().unwrap())?
			.with_engine_shaders()?
			.with_all_fonts()?
			.with_tree_root(engine::ui::make_widget!(ui::root::widget))
			.attach_system(&mut engine, None)?;

		network::Network::send(
			network::packet::Packet::builder()
				.with_address("127.0.0.1:25565")?
				.with_guarantee(
					network::packet::DeliveryGuarantee::Reliable
						+ network::packet::OrderGuarantee::Unordered,
				)
				.with_payload(&packet::Handshake {})
				.build(),
		);
	}

	let engine = engine.into_arclock();
	engine::Engine::run(engine.clone(), || {})
}

struct App();
impl network::NetObserver for App {
	fn on_connect(&mut self, source: std::net::SocketAddr) -> VoidResult {
		if network::mode().contains(network::Kind::Client) {
			log::info!(target: network::LOG, "Connected to {}", source);
			network::Network::send(
				network::packet::Packet::builder()
					.with_address(source)?
					.with_guarantee(
						network::packet::DeliveryGuarantee::Reliable
							+ network::packet::OrderGuarantee::Unordered,
					)
					.with_payload(&packet::Message {
						content: "This is my first message".to_string(),
					})
					.build(),
			);
		}
		Ok(())
	}
}
