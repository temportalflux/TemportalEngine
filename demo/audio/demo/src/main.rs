mod lib;
use lib::*;
fn main() -> anyhow::Result<()> {
	lib::run()?;
	Ok(())
}
