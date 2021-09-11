use super::{DeliveryGuarantee, Guarantee, Kind, OrderGuarantee, Payload};
use std::net::{IpAddr, Ipv4Addr, SocketAddr, ToSocketAddrs};

#[derive(Clone)]
pub struct PacketBuilder {
	address: SocketAddr,
	guarantee: Guarantee,
	payload: Payload,
}

impl Default for PacketBuilder {
	fn default() -> Self {
		use DeliveryGuarantee::*;
		use OrderGuarantee::*;
		Self {
			address: SocketAddr::new(IpAddr::V4(Ipv4Addr::new(127, 0, 0, 1)), 8000),
			guarantee: Unreliable + Unordered,
			payload: Payload::default(),
		}
	}
}

impl PacketBuilder {
	pub fn with_address<T>(mut self, address: T) -> std::io::Result<Self>
	where
		T: ToSocketAddrs,
	{
		self.set_address(address)?;
		Ok(self)
	}

	pub fn set_address<T>(&mut self, address: T) -> std::io::Result<()>
	where
		T: ToSocketAddrs,
	{
		let mut iter = address.to_socket_addrs()?;
		self.address = iter.next().unwrap();
		Ok(())
	}

	pub fn with_guarantee(mut self, guarantee: Guarantee) -> Self {
		self.guarantee = guarantee;
		self
	}

	pub fn with_payload<T>(mut self, payload: &T) -> Self
	where
		T: Kind,
	{
		self.payload = Payload::from(payload);
		self
	}

	pub fn packet_kind(&self) -> &String {
		self.payload.kind()
	}

	pub fn build(self) -> Packet {
		Packet {
			address: self.address,
			guarantee: self.guarantee,
			payload: self.payload,
		}
	}
}

// Mirror of `laminar::Packet`
#[derive(Clone)]
pub struct Packet {
	address: SocketAddr,
	payload: Payload,
	guarantee: Guarantee,
}

impl std::fmt::Debug for Packet {
	fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
		write!(
			f,
			"from:{} guaranteed:({:?}, {:?}) {:?}",
			self.address,
			self.guarantee.delivery(),
			self.guarantee.order(),
			self.payload
		)
	}
}

impl From<laminar::Packet> for Packet {
	fn from(packet: laminar::Packet) -> Self {
		let payload = rmp_serde::from_read_ref::<[u8], Payload>(packet.payload()).unwrap();
		Self {
			address: packet.addr(),
			payload,
			guarantee: Guarantee {
				delivery: packet.delivery_guarantee().into(),
				order: packet.order_guarantee().into(),
			},
		}
	}
}

impl Packet {
	pub fn builder() -> PacketBuilder {
		PacketBuilder::default()
	}

	pub fn into_builder(self) -> PacketBuilder {
		PacketBuilder {
			address: self.address,
			guarantee: self.guarantee,
			payload: self.payload,
		}
	}

	pub fn as_builder(&self) -> PacketBuilder {
		self.clone().into_builder()
	}

	pub fn address(&self) -> &SocketAddr {
		&self.address
	}

	pub fn guarantees(&self) -> &Guarantee {
		&self.guarantee
	}

	pub fn kind(&self) -> &String {
		self.payload.kind()
	}

	pub fn take_payload(&mut self) -> Payload {
		self.payload.take()
	}
}

impl Into<laminar::Packet> for Packet {
	fn into(self) -> laminar::Packet {
		use DeliveryGuarantee::*;
		use OrderGuarantee::*;
		let raw_payload = rmp_serde::to_vec(&self.payload).unwrap();
		match self.guarantee {
			Guarantee {
				delivery: Unreliable,
				order: Unordered,
			} => laminar::Packet::unreliable(self.address, raw_payload),
			Guarantee {
				delivery: Unreliable,
				order: Sequenced,
			} => laminar::Packet::unreliable_sequenced(self.address, raw_payload, None),
			Guarantee {
				delivery: Reliable,
				order: Unordered,
			} => laminar::Packet::reliable_unordered(self.address, raw_payload),
			Guarantee {
				delivery: Reliable,
				order: Sequenced,
			} => laminar::Packet::reliable_sequenced(self.address, raw_payload, None),
			Guarantee {
				delivery: Reliable,
				order: Ordered,
			} => laminar::Packet::reliable_ordered(self.address, raw_payload, None),
			_ => panic!("Invalid guarantee {:?}", self.guarantee),
		}
	}
}
