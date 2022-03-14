use std::path::PathBuf;

#[derive(Clone)]
pub struct BuildPath {
	pub source: PathBuf,
	pub relative: PathBuf,
	pub destination: PathBuf,
}

impl BuildPath {
	pub fn source_with_ext(&self, ext: &str) -> PathBuf {
		let mut path = self.source.parent().unwrap().to_path_buf();
		path.push(self.source.file_stem().unwrap());
		path.set_extension(ext);
		path
	}

	pub fn file_name(&self) -> &str {
		self.source.file_name().unwrap().to_str().unwrap()
	}
}
