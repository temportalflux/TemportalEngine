mod lib;
use lib::*;
fn main() -> engine::utility::Result<()> {
	lib::run()?;
	Ok(())
}
