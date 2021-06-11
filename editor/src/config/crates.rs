use crate::engine::utility::{AnyError, SaveData};
use serde::{Deserialize, Serialize};
use std::path::{Path, PathBuf};
use std::sync::{LockResult, RwLockReadGuard, RwLockWriteGuard};

#[derive(Debug, Default, Deserialize, Serialize)]
pub struct Crates {
	// TODO: Each entry should specify where its pak file should be outputted
	#[serde(default)]
	crates: Vec<PathBuf>,
}

impl SaveData<Crates> for Crates {
	fn path() -> &'static str {
		"config/crates.json"
	}
	fn from_json(json: &str) -> Result<Self, AnyError> {
		let value: Self = serde_json::from_str(json)?;
		Ok(value)
	}
	fn to_json(&self) -> Result<String, AnyError> {
		let json = serde_json::to_string_pretty(self)?;
		Ok(json)
	}
}

impl Crates {
	fn get() -> &'static std::sync::RwLock<Self> {
		use crate::engine::utility::singleton::*;
		static mut INSTANCE: Singleton<Crates> = Singleton::uninit();
		unsafe {
			INSTANCE.get_or_init(|| match Self::load() {
				Ok(inst) => {
					inst.save().unwrap();
					inst
				}
				Err(e) => {
					log::error!(
						target: crate::LOG,
						"Failed to load crates configuration: {}",
						e
					);
					Self::default()
				}
			})
		}
	}

	pub fn write() -> LockResult<RwLockWriteGuard<'static, Self>> {
		Self::get().write()
	}

	pub fn read() -> LockResult<RwLockReadGuard<'static, Self>> {
		Self::get().read()
	}
}

#[derive(Debug)]
pub struct Manifest {
	pub name: String,
	pub version: semver::Version,
	pub location: PathBuf,
}

impl Manifest {
	pub fn parse(directory: &Path) -> Option<Manifest> {
		let manifest_path = directory.join("Cargo.toml");
		match std::fs::read_to_string(&manifest_path) {
			Ok(raw) => match cargo_toml::Manifest::from_str(&raw) {
				Ok(manifest) => {
					if let Some(package) = &manifest.package {
						return Some(Manifest {
							name: package.name.clone(),
							version: semver::Version::parse(&package.version).unwrap(),
							location: directory.canonicalize().unwrap(),
						});
					} else {
						log::error!(
							target: crate::LOG,
							"Cannot include manifest {}, it is not a package.",
							manifest_path.to_str().unwrap()
						);
					}
				}
				Err(e) => {
					log::error!(
						target: crate::LOG,
						"Failed to parse editor crate manifest: {}",
						e
					);
				}
			},
			Err(e) => {
				log::error!(
					target: crate::LOG,
					"Failed to load editor crate manifest: {}",
					e
				);
			}
		}
		return None;
	}
}

impl Crates {
	pub fn manifests(&self) -> Vec<Manifest> {
		let mut manifests = Vec::new();
		for path in self.crates.iter() {
			let dir = std::env::current_dir().unwrap().join(path);
			if let Some(manifest) = Manifest::parse(&dir) {
				manifests.push(manifest);
			}
		}
		manifests
	}
}
