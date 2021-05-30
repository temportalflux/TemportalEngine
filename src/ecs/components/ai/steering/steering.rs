use crate::math::{Quaternion, Vector};

pub type Id = &'static str;
pub type IdOwned = String;

pub trait Behavior {
	fn name() -> Id;
	fn name_owned() -> IdOwned {
		Self::name().to_owned()
	}
	fn get_steering(&mut self, state: &State) -> Output;
}

pub struct State {
	pub position: Vector<f32, 3>,
	pub velocity: Vector<f32, 3>,
	pub orientation: Quaternion,
	pub rotation: f32,
}

#[derive(Default, Debug, Clone, Copy)]
pub struct Output {
	pub linear_acceleration: Vector<f32, 3>,
	pub angular_acceleration: f32,
}

impl std::ops::AddAssign for Output {
	fn add_assign(&mut self, other: Self) {
		self.linear_acceleration += other.linear_acceleration;
		self.angular_acceleration += other.angular_acceleration;
	}
}

impl std::ops::Mul<f32> for Output {
	type Output = Self;
	fn mul(self, other: f32) -> Self::Output {
		Self {
			linear_acceleration: self.linear_acceleration * other,
			angular_acceleration: self.angular_acceleration * other,
		}
	}
}
