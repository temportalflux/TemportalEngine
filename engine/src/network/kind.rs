use enumset::{EnumSet, EnumSetType};

#[derive(Debug, EnumSetType)]
pub enum Kind {
	Client,
	Server,
}

pub type KindSet = EnumSet<Kind>;
