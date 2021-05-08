use crate::{utility::VoidResult, Application};
pub use log::Level;

pub fn init<T: Application>() -> VoidResult {
	init_named(T::name())
}

pub fn init_named(log_name: &str) -> VoidResult {
	optick::event!();
	use simplelog::*;
	let mut log_path = std::env::current_dir()?.to_path_buf();
	log_path.push(format!("{}.log", log_name));
	let file = std::fs::OpenOptions::new()
		.create(true)
		.write(true)
		.truncate(true)
		.open(log_path)?;
	let cfg = ConfigBuilder::new()
		.set_max_level(log::LevelFilter::Error)
		.set_time_format_str("%Y.%m.%d-%H.%M.%S")
		.set_thread_level(log::LevelFilter::Debug)
		.set_target_level(log::LevelFilter::Error)
		.set_location_level(log::LevelFilter::Off)
		.build();
	CombinedLogger::init(vec![
		TermLogger::new(
			LevelFilter::Trace,
			cfg.clone(),
			TerminalMode::Mixed,
			ColorChoice::Auto,
		),
		WriteLogger::new(LevelFilter::Trace, cfg.clone(), file),
	])
	.unwrap();
	Ok(())
}
