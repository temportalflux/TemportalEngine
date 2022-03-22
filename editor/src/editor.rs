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

	pub async fn run_commandlets() -> bool {
		let (should_build_assets, force_build, should_package_assets) = {
			let should_build_assets = std::env::args().any(|arg| arg == "-build-assets");
			let force_build = std::env::args().any(|arg| arg == "-force");
			let should_package_assets = std::env::args().any(|arg| arg == "-package");
			(should_build_assets, force_build, should_package_assets)
		};

		if should_build_assets {
			if let Err(errors) = Self::build_assets(force_build).await {
				for error in errors.into_iter() {
					log::error!(target: "editor", "{error:?}");
				}
			}
		}

		if should_package_assets {
			if let Err(errors) = Self::package_assets().await {
				for error in errors.into_iter() {
					log::error!(target: "editor", "{error:?}");
				}
			}
		}

		should_build_assets || should_package_assets
	}

	pub async fn build_assets(force_build: bool) -> Result<(), Vec<anyhow::Error>> {
		let (modules, manager) = {
			let editor = Self::read();
			(editor.asset_modules.clone(), editor.asset_manager.clone())
		};
		asset::Module::build_all(modules, manager, force_build).await
	}

	pub async fn package_assets() -> Result<(), Vec<anyhow::Error>> {
		let paks = {
			let editor = Self::read();
			editor.paks.clone()
		};
		asset::Pak::package_all(paks).await
	}
}
