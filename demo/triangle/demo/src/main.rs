mod lib;
pub use lib::*;
fn main() -> engine::utility::Result<()> {
	lib::run()
}
