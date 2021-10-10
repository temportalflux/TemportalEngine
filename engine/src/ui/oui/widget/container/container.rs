use super::Slot;

pub struct Container<TSlot: Slot> {
	slots: Vec<TSlot>,
}

impl<TSlot> Container<TSlot>
where
	TSlot: Slot,
{
	pub fn new() -> Self {
		Self { slots: Vec::new() }
	}

	pub fn push(&mut self, slot: TSlot) {
		self.slots.push(slot);
	}

	pub fn iter(&self) -> impl std::iter::Iterator<Item = &TSlot> + '_ {
		self.slots.iter()
	}
}
