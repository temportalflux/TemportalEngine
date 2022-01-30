use crate::utility::Result;
pub use log::Level;

pub fn default_path(app_name: &str, suffix: Option<&str>) -> std::path::PathBuf {
	let mut log_path = std::env::current_dir().unwrap().to_path_buf();
	log_path.push(format!(
		"{}{}{}.log",
		app_name,
		suffix.unwrap_or(""),
		std::env::args()
			.find_map(|arg| arg.strip_prefix("-log-suffix=").map(|s| s.to_string()))
			.unwrap_or("".to_string())
	));
	log_path
}

#[profiling::function]
pub fn init(log_path: &std::path::Path) -> Result<()> {
	use simplelog::*;
	if let Some(parent) = log_path.parent() {
		std::fs::create_dir_all(parent)?;
	}
	let file = std::fs::OpenOptions::new()
		.create(true)
		.write(true)
		.truncate(true)
		.open(log_path)?;
	let cfg = {
		let mut builder = ConfigBuilder::new();
		builder
			.set_max_level(log::LevelFilter::Error)
			.set_time_format_str("%Y.%m.%d-%H.%M.%S")
			// Pads the names of levels so that they line up in the log.
			// [ERROR]
			// [ WARN]
			// [ INFO]
			// [DEBUG]
			// [TRACE]
			.set_level_padding(LevelPadding::Left)
			// Thread IDs/Names are logged for ALL statements (that aren't on main)
			.set_thread_level(log::LevelFilter::Error)
			.set_thread_mode(ThreadLogMode::Names)
			.set_thread_padding(ThreadPadding::Left(5))
			// Target is always logged so that readers know what owner logged each line
			.set_target_level(log::LevelFilter::Error)
			.set_location_level(log::LevelFilter::Off);
		for input_dep in crate::input::DEPENDENCY_LOG_TARGETS.iter() {
			builder.add_filter_ignore_str(input_dep);
		}
		builder.add_filter_ignore_str("mio"); // for quinn networking
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
