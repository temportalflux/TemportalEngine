use crate::{asset, settings};
use anyhow::Result;
use engine::{
	self,
	utility::{singleton::Singleton, SaveData},
	Application,
};
use std::sync::{atomic::AtomicBool, Arc};

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
		let build = std::env::args().any(|arg| arg == "-build-assets");
		let forced = std::env::args().any(|arg| arg == "-force");
		let package = std::env::args().any(|arg| arg == "-package");
		let _ = Self::build_and_package(build, package, forced, None).await;
		build || package
	}

	pub fn build_and_package(
		build: bool,
		package: bool,
		forced: bool,
		async_is_active: Option<Arc<AtomicBool>>,
	) -> engine::task::JoinHandle<()> {
		use engine::task::{global_scopes, scope::*};
		let task_scope = Scope::named("Build & Package", "build-package");
		let unattached = task_scope.clone().spawn_silently(async move {
			if let Some(is_active) = &async_is_active {
				assert!(!is_active.load(std::sync::atomic::Ordering::Acquire));
			}

			// Build the assets (possibly forcibly).
			let mut all_modules_successful = true;
			if build {
				let scope_build_all = Scope::named("Build All", "build-all");
				let unattached = scope_build_all.clone().spawn(async move {
					let (modules, manager) = {
						let editor = Self::read();
						(editor.asset_modules.clone(), editor.asset_manager.clone())
					};

					let mut subscopes = Vec::with_capacity(modules.len());
					for module in modules.into_iter() {
						let scope = Scope::named(
							format!("Build {}", module.name),
							format!("build({})", module.name),
						);
						let attached = scope
							.spawn(module.build(manager.clone(), forced))
							.attach_to(&scope_build_all);
						subscopes.push(attached);
					}

					Ok(engine::task::join_all(subscopes).await?)
				});
				let attached = unattached.attach_to(&task_scope);
				all_modules_successful = attached.await??;
			}

			// Pacakge the assets
			if package && all_modules_successful {
				let scope_package_all = Scope::named("Package All", "package-all");
				let unattached = scope_package_all.clone().spawn(async move {
					let paks = {
						let editor = Self::read();
						editor.paks.clone()
					};
					let mut subscopes = Vec::with_capacity(paks.len());
					for pak in paks.into_iter() {
						let scope = Scope::named(
							format!("Package {}", pak.name),
							format!("package({})", pak.name),
						);
						let attached = scope
							.spawn(async move { pak.package().await })
							.attach_to(&scope_package_all);
						subscopes.push(attached);
					}
					Ok(engine::task::join_all(subscopes).await?)
				});
				let _ = unattached.attach_to(&task_scope).await??;
			}

			if let Some(is_active) = &async_is_active {
				// There were no errors promoted above, so we are guarunteed to always
				// mark the flag as no-build-active when the operations have finished.
				is_active.store(false, std::sync::atomic::Ordering::Relaxed);
			}

			Ok(())
		});
		unattached.attach_to(global_scopes())
	}
}
