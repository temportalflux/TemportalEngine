#[derive(Debug, Clone, Copy)]
pub enum Archetype {
	Wander,
	Flocking,
}

impl Archetype {
	pub fn all() -> Vec<Self> {
		vec![Self::Wander, Self::Flocking]
	}

	pub fn display_name(&self) -> &'static str {
		match self {
			Self::Wander => "Wander",
			Self::Flocking => "Flocking",
		}
	}
}
