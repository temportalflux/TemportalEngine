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
}
