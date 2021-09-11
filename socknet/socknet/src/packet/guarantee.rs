#[derive(Debug, Clone, Copy)]
pub enum DeliveryGuarantee {
	Reliable,
	Unreliable,
}

impl From<laminar::DeliveryGuarantee> for DeliveryGuarantee {
	fn from(kind: laminar::DeliveryGuarantee) -> Self {
		use laminar::DeliveryGuarantee::*;
		match kind {
			Unreliable => Self::Unreliable,
			Reliable => Self::Reliable,
		}
	}
}

#[derive(Debug, Clone, Copy)]
pub enum OrderGuarantee {
	Unordered,
	Sequenced,
	Ordered,
}

impl From<laminar::OrderingGuarantee> for OrderGuarantee {
	fn from(kind: laminar::OrderingGuarantee) -> Self {
		use laminar::OrderingGuarantee::*;
		match kind {
			None => Self::Unordered,
			Sequenced(_stream_id) => Self::Sequenced,
			Ordered(_stream_id) => Self::Ordered,
		}
	}
}

/// See https://docs.rs/laminar/0.5.0/laminar/struct.Packet.html for more on the kinds of guarantees.
#[derive(Debug, Clone, Copy)]
pub struct Guarantee {
	pub(super) delivery: DeliveryGuarantee,
	pub(super) order: OrderGuarantee,
}

impl std::fmt::Display for Guarantee {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		write!(f, "Guarantee({:?}, {:?})", self.delivery, self.order)
	}
}

impl std::ops::Add<DeliveryGuarantee> for OrderGuarantee {
	type Output = Guarantee;
	fn add(self, other: DeliveryGuarantee) -> Self::Output {
		Guarantee::new(other, self)
	}
}

impl std::ops::Add<OrderGuarantee> for DeliveryGuarantee {
	type Output = Guarantee;
	fn add(self, other: OrderGuarantee) -> Self::Output {
		Guarantee::new(self, other)
	}
}

impl Guarantee {
	pub fn new(delivery: DeliveryGuarantee, order: OrderGuarantee) -> Self {
		let guarantee = Self { delivery, order };
		if !guarantee.is_valid() {
			panic!("Invalid guarantee combination {:?}", guarantee);
		}
		guarantee
	}

	pub fn is_valid(&self) -> bool {
		use DeliveryGuarantee::*;
		use OrderGuarantee::*;
		match (self.delivery, self.order) {
			(Unreliable, Unordered) => true,
			(Unreliable, Sequenced) => true,
			(Reliable, Unordered) => true,
			(Reliable, Sequenced) => true,
			(Reliable, Ordered) => true,
			_ => false,
		}
	}

	pub fn delivery(&self) -> DeliveryGuarantee {
		self.delivery
	}

	pub fn order(&self) -> OrderGuarantee {
		self.order
	}
}
