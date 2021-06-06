pub trait Application {
	/// Returns the name of the crate package via `std::env!("CARGO_PKG_NAME")`.
	fn name() -> &'static str;

	/// Returns the user-facing display name of the package, which may not match the package name at all.
	fn display_name() -> &'static str;

	/// Returns the name of the crate package via `std::env!("CARGO_MANIFEST_DIR")`.
	/// This is useful for editor to know relative path locations for assets and binaries.
	fn location() -> &'static str;

	/// Returns a packed version integer by converting `std::env!("CARGO_PKG_VERSION")` using `utility::make_version`.
	fn version() -> u32;

	/// Returns the id of an asset which may exist in the module assets at a given path.
	/// Delegates to [`Id::new(Self::name(), path)`](crate::asset::Id::new).
	fn get_asset_id(path: &str) -> crate::asset::Id {
		crate::asset::Id::new(Self::name(), path)
	}
}
