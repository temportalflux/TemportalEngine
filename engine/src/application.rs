pub trait Application {
	/// Returns the name of the crate package via `std::env!("CARGO_PKG_NAME")`.
	fn name() -> &'static str;

	/// Returns a packed version integer by converting `std::env!("CARGO_PKG_VERSION")` using `utility::make_version`.
	fn version() -> semver::Version;

	fn version_int() -> u32 {
		let v = Self::version();
		crate::utility::make_version(v.major, v.minor, v.patch)
	}

	/// Returns the id of an asset which may exist in the module assets at a given path.
	/// Delegates to [`Id::new(Self::name(), path)`](crate::asset::Id::new).
	fn get_asset_id(path: &str) -> crate::asset::Id {
		crate::asset::Id::new(Self::name(), path)
	}
}
