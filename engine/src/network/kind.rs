use enumset::EnumSetType;

#[derive(Debug, EnumSetType)]
pub enum Kind {
	Client,
	Server,
}
