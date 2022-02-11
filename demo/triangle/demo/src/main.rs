mod lib;
pub use lib::*;
fn main() -> anyhow::Result<()> {
	lib::run()
}
