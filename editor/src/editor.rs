use crate::{asset, settings};
use anyhow::Result;
use engine::{
	self,
	utility::{singleton::Singleton, SaveData},
	Application,
};
use std::sync::Arc;

pub static EDITOR_LOG: &'static str = "Editor";

pub struct Editor {
	asset_manager: Arc<asset::Manager>,
	pub settings: settings::Editor,
	pub asset_modules: Vec<Arc<asset::Module>>,
	pub paks: Vec<Arc<asset::Pak>>,
}

impl Application for Editor {
	fn name() -> &'static str {
		std::env!("CARGO_PKG_NAME")
	}
	fn version() -> semver::Version {
		semver::Version::parse(std::env!("CARGO_PKG_VERSION")).unwrap()
	}
}

impl Editor {
	unsafe fn instance() -> &'static mut Singleton<Self> {
		static mut INSTANCE: Singleton<Editor> = Singleton::uninit();
		&mut INSTANCE
	}

	pub fn initialize(editor: Self) -> Result<()> {
		unsafe { Self::instance() }.init_with(editor);
		Ok(())
	}

	fn get() -> &'static std::sync::RwLock<Self> {
		unsafe { Self::instance() }.get()
	}

	pub fn read() -> std::sync::RwLockReadGuard<'static, Self> {
		Self::get().read().unwrap()
	}

	pub fn write() -> std::sync::RwLockWriteGuard<'static, Self> {
		Self::get().write().unwrap()
	}

	pub async fn new(asset_manager: asset::Manager) -> Result<Self> {
		log::info!(target: EDITOR_LOG, "Initializing editor");

		let mut editor = Self {
			asset_manager: Arc::new(asset_manager),
			settings: settings::Editor::load()?,
			asset_modules: Vec::new(),
			paks: Vec::new(),
		};
		engine::asset::Library::scan_pak_directory().await?;

		if let Ok(crates_cfg) = crate::config::Crates::read() {
			for manifest in crates_cfg.manifests().into_iter() {
				editor.add_crate_manifest(&manifest);
			}
		}

		Ok(editor)
	}

	fn add_crate_manifest(&mut self, manifest: &crate::config::Manifest) {
		let module_idx = self.asset_modules.len();
		self.asset_modules.push(Arc::new(asset::Module {
			name: manifest.name.clone(),
			assets_directory: manifest.location.join("assets"),
			binaries_directory: manifest.location.join("binaries"),
		}));
		self.paks.push(Arc::new(asset::Pak {
			name: manifest.name.clone(),
			binaries_directory: manifest.location.join("binaries"),
			output_directories: manifest.config.pak_destinations.clone(),
			modules: vec![module_idx],
		}));
	}

	pub fn run_commandlets() -> Option<engine::task::JoinHandle<()>> {
		let mut args = std::env::args();
		let should_build_assets = args.any(|arg| arg == "-build-assets");
		let should_package_assets = args.any(|arg| arg == "-package");
		if should_build_assets || should_package_assets {
			let force_build = args.any(|arg| arg == "-force");
			let handle = engine::task::spawn("editor".to_owned(), async move {
				if should_build_assets {
					let handles = {
						let mut module_tasks = Vec::new();
						let editor = Self::read();
						for module in editor.asset_modules.iter() {
							let async_module = module.clone();
							let async_asset_manager = editor.asset_manager.clone();
							module_tasks.push(engine::task::spawn(
								"editor".to_owned(),
								async move {
									async_module
										.build_async(async_asset_manager, force_build)
										.await
								},
							));
						}
						module_tasks
					};
					futures::future::join_all(handles.into_iter()).await;
				}

				if should_package_assets {
					let handles = {
						let mut module_tasks = Vec::new();
						let editor = Self::read();
						for pak in editor.paks.iter() {
							let async_pak = pak.clone();
							let task = engine::task::spawn("editor".to_owned(), async move {
								async_pak.package().await
							});
							module_tasks.push(task);
						}
						module_tasks
					};
					futures::future::join_all(handles.into_iter()).await;
				}
				Ok(())
			});
			return Some(handle);
		}
		return None;
	}
}
