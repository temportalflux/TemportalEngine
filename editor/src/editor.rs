use crate::{
	asset,
	engine::{
		self,
		utility::{singleton::Singleton, AnyError, SaveData, VoidResult},
		Application, EngineApp,
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
	fn display_name() -> &'static str {
		"Crystal Sphinx"
	}
	fn location() -> &'static str {
		std::env!("CARGO_MANIFEST_DIR")
	}
	fn version() -> u32 {
		engine::utility::make_version(
			std::env!("CARGO_PKG_VERSION_MAJOR").parse().unwrap(),
			std::env!("CARGO_PKG_VERSION_MINOR").parse().unwrap(),
			std::env!("CARGO_PKG_VERSION_PATCH").parse().unwrap(),
		)
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
		use std::path::PathBuf;
		log::info!(target: EDITOR_LOG, "Initializing editor");
		let mut editor = Self {
			asset_manager: asset::Manager::new(),
			settings: settings::Editor::load()?,
			asset_modules: Vec::new(),
			paks: Vec::new(),
		};
		crate::graphics::register_asset_types(&mut editor.asset_manager);
		engine::asset::Library::write().scan_pak_directory()?;

		let editor_path = PathBuf::from(Editor::location());
		editor.add_asset_module(asset::Module::from_app::<Editor>(&editor_path));
		editor.add_pak(asset::Pak::from_app::<Editor>(&editor_path, None));

		let output_directory = editor.settings.packager_output().clone();

		let engine_path = PathBuf::from(EngineApp::location());
		editor.add_asset_module(asset::Module::from_app::<EngineApp>(&engine_path));
		editor.add_pak(asset::Pak::from_app::<EngineApp>(
			&engine_path,
			Some(&output_directory),
		));

		let module_path = PathBuf::from(T::location());
		editor.add_asset_module(asset::Module::from_app::<T>(&module_path));
		editor.add_pak(asset::Pak::from_app::<T>(
			&module_path,
			Some(&output_directory),
		));

		Ok(editor)
	}

	pub fn add_asset_module(&mut self, module: asset::Module) {
		self.asset_modules.push(module);
	}

	pub fn add_pak(&mut self, pak: asset::Pak) {
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
