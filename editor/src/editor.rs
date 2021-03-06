use crate::{
	asset,
	engine::{
		self,
		utility::{singleton::Singleton, AnyError, SaveData, VoidResult},
		Application,
	},
	settings,
};

pub static EDITOR_LOG: &'static str = "Editor";

pub struct Editor {
	asset_manager: asset::Manager,
	pub settings: settings::Editor,
	pub asset_modules: Vec<asset::Module>,
	pub paks: Vec<asset::Pak>,
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

	pub fn initialize<T: Application>() -> VoidResult {
		unsafe { Self::instance() }.init_with(Self::new::<T>()?);
		Ok(())
	}

	pub fn get() -> &'static std::sync::RwLock<Self> {
		unsafe { Self::instance() }.get()
	}

	pub fn read() -> std::sync::RwLockReadGuard<'static, Self> {
		Self::get().read().unwrap()
	}

	pub fn write() -> std::sync::RwLockWriteGuard<'static, Self> {
		Self::get().write().unwrap()
	}

	fn new<T: Application>() -> Result<Self, AnyError> {
		log::info!(target: EDITOR_LOG, "Initializing editor");
		let mut editor = Self {
			asset_manager: asset::Manager::new(),
			settings: settings::Editor::load()?,
			asset_modules: Vec::new(),
			paks: Vec::new(),
		};
		crate::audio::register_asset_types(&mut editor.asset_manager);
		crate::graphics::register_asset_types(&mut editor.asset_manager);
		engine::asset::Library::write().scan_pak_directory()?;

		if let Ok(crates_cfg) = crate::config::Crates::read() {
			for manifest in crates_cfg.manifests().into_iter() {
				editor.add_crate_manifest(&manifest);
			}
		}

		Ok(editor)
	}

	fn add_crate_manifest(&mut self, manifest: &crate::config::Manifest) {
		self.add_asset_module(asset::Module {
			name: manifest.name.clone(),
			assets_directory: manifest.location.join("assets"),
			binaries_directory: manifest.location.join("binaries"),
		});
		self.add_pak(asset::Pak {
			name: manifest.name.clone(),
			binaries_directory: manifest.location.join("binaries"),
			output_directory: manifest.config.pak_destination.clone(),
		});
	}

	pub fn add_asset_module(&mut self, module: asset::Module) {
		log::info!(
			target: crate::LOG,
			"Adding asset module \"{}\"",
			module.name
		);
		self.asset_modules.push(module);
	}

	pub fn add_pak(&mut self, pak: asset::Pak) {
		log::info!(target: crate::LOG, "Adding pak \"{}\"", pak.name);
		self.paks.push(pak);
	}

	pub fn asset_manager(&self) -> &asset::Manager {
		&self.asset_manager
	}

	pub fn asset_manager_mut(&mut self) -> &mut asset::Manager {
		&mut self.asset_manager
	}

	pub fn run_commandlets(&self) -> Result<bool, AnyError> {
		let mut args = std::env::args();
		let should_build_assets = args.any(|arg| arg == "-build-assets");
		let should_package_assets = args.any(|arg| arg == "-package");
		if should_build_assets || should_package_assets {
			if should_build_assets {
				for module in self.asset_modules.iter() {
					module.build(self.asset_manager(), args.any(|arg| arg == "-force"))?;
				}
			}
			if should_package_assets {
				for pak in self.paks.iter() {
					pak.package()?;
				}
			}
			return Ok(true);
		}
		return Ok(false);
	}
}
