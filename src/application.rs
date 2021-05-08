pub trait Application {
	fn name() -> &'static str;
	fn display_name() -> &'static str;
	fn location() -> &'static str;
	fn version() -> u32;
}
