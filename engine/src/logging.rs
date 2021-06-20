use crate::utility::VoidResult;
pub use log::Level;

#[profiling::function]
pub fn init(app_name: &str, suffix: Option<&str>) -> VoidResult {
	use simplelog::*;
	let log_name = format!("{}{}", app_name, suffix.unwrap_or(""));
	let mut log_path = std::env::current_dir()?.to_path_buf();
	log_path.push(format!("{}.log", log_name));
	let file = std::fs::OpenOptions::new()
		.create(true)
		.write(true)
		.truncate(true)
		.open(&log_path)?;
	let cfg = {
		let mut builder = ConfigBuilder::new();
		builder
			.set_max_level(log::LevelFilter::Error)
			.set_time_format_str("%Y.%m.%d-%H.%M.%S")
			.set_thread_level(log::LevelFilter::Debug)
			.set_target_level(log::LevelFilter::Error)
			.set_location_level(log::LevelFilter::Off);
		for input_dep in crate::input::DEPENDENCY_LOG_TARGETS.iter() {
			builder.add_filter_ignore_str(input_dep);
		}
		builder.build()
	};
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
	log::info!(target: "engine", "Writing log to {}", log_path.display());
	log::info!(target: "engine", "Executing: {:?}", std::env::args().collect::<Vec<_>>());
	Ok(())
}
