use crate::ecs::{self, Component, VecStorage};
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Neighborhood {
	/// How aligned another entity must be with the forward vector
	/// of this entity to be considered inside the neighborhood.
	///
	/// The makes up the sides of the cone.
	/// A value of 0 means it must be perfectly aligned (not recommended),
	/// 1.0 means "anywhere forward"/infinite cone,
	/// 0.5 would represent a cone at 45 degrees.
	///
	/// An entity is aligned if `1.0 - dot(self.forward, (other.position - self.position).normalized()) <= alignment`,
	/// aka "the other entity's position within an angle of separation from the forward vector of this entity".
	alignment: f32,
	/// How far in front of the entity can an entity be
	/// to still be considered inside the neighborhood.
	max_distance: f32,

	/// The ids of entities inside the neighborhood.
	/// Any of these entities may not exist anymore.
	pub(crate) neighbors: Vec<ecs::world::Index>,
}

impl Component for Neighborhood {
	type Storage = VecStorage<Self>;
}

impl Default for Neighborhood {
	fn default() -> Self {
		Self {
			alignment: 0.5,
			max_distance: 1.0,
			neighbors: Vec::new(),
		}
	}
}

impl Neighborhood {
	pub fn with_alignment(mut self, alignment: f32) -> Self {
		self.alignment = alignment;
		self
	}

	pub fn with_max_distance(mut self, max_distance: f32) -> Self {
		self.max_distance = max_distance;
		self
	}

	pub fn is_aligned(&self, alignment: f32) -> bool {
		alignment <= self.alignment
	}

	pub fn is_within_distance(&self, distance_sq: f32) -> bool {
		distance_sq <= self.max_distance.powi(2)
	}
}
