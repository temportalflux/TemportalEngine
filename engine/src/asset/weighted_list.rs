use crate::asset::Id;

#[derive(Debug, Default, Clone)]
pub struct WeightedIdList {
	ids: Vec<(usize, Id)>,
	total_weight: usize,
}

impl WeightedIdList {
	pub fn insert(&mut self, weight: usize, id: Id) {
		self.ids.push((weight, id));
		self.total_weight += weight;
	}

	pub fn total_weight(&self) -> usize {
		self.total_weight
	}

	pub fn pick(&self, amount: usize) -> Option<&Id> {
		let mut amount = amount;
		let mut iter = self.ids.iter();
		while let Some((weight, id)) = iter.next() {
			if amount < *weight {
				return Some(id);
			}
			amount -= *weight;
		}
		None
	}
}
