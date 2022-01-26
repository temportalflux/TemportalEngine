use crate::graphics::device::physical;

pub use anyhow::{Context, Error, Result};

pub struct FailedToFindPhysicalDevice(pub Option<physical::Constraint>);
impl std::error::Error for FailedToFindPhysicalDevice {}
impl std::fmt::Debug for FailedToFindPhysicalDevice {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		<Self as std::fmt::Display>::fmt(&self, f)
	}
}
impl std::fmt::Display for FailedToFindPhysicalDevice {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		match &self.0 {
			Some(constraint) => write!(
				f,
				"Failed to find physical device due to constraint {:?}",
				constraint
			),
			None => write!(f, "Failed to find any physical devices, do you have a GPU?"),
		}
	}
}
