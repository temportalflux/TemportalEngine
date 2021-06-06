#[derive(Default, Debug, Clone, Copy)]
pub struct DeltaTime(pub std::time::Duration);

impl DeltaTime {
	pub fn get(&self) -> std::time::Duration {
		self.0
	}
}
