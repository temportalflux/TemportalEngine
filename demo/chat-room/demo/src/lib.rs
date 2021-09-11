use engine::{network, utility::VoidResult, Application};
pub use temportal_engine as engine;

#[path = "packet/mod.rs"]
pub mod packet;

#[path = "ui/mod.rs"]
pub mod ui;

#[path = "message_history.rs"]
mod message_history;
pub use message_history::*;

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
	let is_server = std::env::args().any(|arg| arg == "-server");
	let is_client = std::env::args().any(|arg| arg == "-client");
	let log_path = {
		let mut log_path = std::env::current_dir().unwrap().to_path_buf();
		log_path.push(if is_server { "server" } else { "client" });
		log_path.push(format!("{}.log", ChatRoom::name()));
		log_path
	};
	engine::logging::init(&log_path)?;
	let mut engine = engine::Engine::new()?;
	engine.scan_paks()?;

	let network_port = std::env::args()
		.find_map(|arg| {
			arg.strip_prefix("-port=")
				.map(|s| s.parse::<u16>().ok())
				.flatten()
		})
		.unwrap_or(25565);

	let mut network_builder = network::Builder::default().with_port(network_port);
	if is_server {
		network_builder.insert_modes(network::mode::Kind::Server);
	}
	if is_client {
		network_builder.insert_modes(network::mode::Kind::Client);
	}
	packet::register_types(&mut network_builder);
	network_builder.spawn()?;

	if is_client {
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
		)?;
	}

	let engine = engine.into_arclock();
	engine::Engine::run(engine.clone(), || {})
}
