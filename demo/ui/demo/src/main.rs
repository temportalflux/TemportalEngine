mod lib;
fn main() -> anyhow::Result<()> {
	lib::run()?;
	Ok(())
}
