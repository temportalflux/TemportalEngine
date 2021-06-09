use crate::{
	asset,
	engine::{
		self,
		utility::{singleton::Singleton, AnyError, SaveData, VoidResult},
		Application, EngineApp,
	},
	settings::{self},
};
use std::path::PathBuf;

pub static EDITOR_LOG: &'static str = "Editor";

/// Editor-level wrapper for [`Application`] objects which are present in this runtime.
pub struct ApplicationModule {
	pub name: String,
	pub location: PathBuf,
	pub is_editor_only: bool,
}

impl ApplicationModule {
	pub fn new<T: Application>() -> Self {
		Self {
			name: T::name().to_string(),
			location: PathBuf::from(T::location()),
			is_editor_only: T::is_editor_only(),
		}
	}
}

pub struct Editor {
	asset_manager: asset::Manager,
	pub settings: settings::Editor,
	pub modules: Vec<ApplicationModule>,
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
	fn is_editor_only() -> bool {
		true
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
			modules: vec![
				ApplicationModule::new::<EngineApp>(),
				ApplicationModule::new::<Editor>(),
				ApplicationModule::new::<T>(),
			],
			paks: Vec::new(),
		};
		crate::graphics::register_asset_types(&mut editor.asset_manager);
		engine::asset::Library::write().scan_pak_directory()?;

		editor.add_pak(asset::Pak::from_app::<Editor>(None));

		let output_directory = editor.settings.packager_output().clone();
		editor.add_pak(asset::Pak::from_app::<EngineApp>(Some(&output_directory)));
		editor.add_pak(asset::Pak::from_app::<T>(Some(&output_directory)));

		Ok(editor)
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
			for app_module in self.modules.iter() {
				if should_build_assets {
					asset::build(
						self.asset_manager(),
						&app_module.name,
						&app_module.location,
						args.any(|arg| arg == "-force"),
					)?;
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
