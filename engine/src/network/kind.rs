use enumset::{EnumSet, EnumSetType};

#[derive(Debug, EnumSetType)]
pub enum Kind {
	Client,
	Server,
}

pub type KindSet = EnumSet<Kind>;

impl std::fmt::Display for Kind {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		match self {
			Self::Client => write!(f, "Client"),
			Self::Server => write!(f, "Server"),
		}
	}
}

impl std::ops::Add<Kind> for Kind {
	type Output = KindSet;
	fn add(self, other: Kind) -> Self::Output {
		let mut set = KindSet::new();
		set.insert(self);
		set.insert(other);
		set
	}
}

impl std::ops::Add<KindSet> for Kind {
	type Output = KindSet;
	fn add(self, other: KindSet) -> Self::Output {
		let mut set = KindSet::new();
		set.insert_all(other);
		set.insert(self);
		set
	}
}
