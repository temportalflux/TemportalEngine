use crate::math::nalgebra::{Point3, UnitQuaternion, Vector3};

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
	pub position: Point3<f32>,
	pub velocity: Vector3<f32>,
	pub orientation: UnitQuaternion<f32>,
	pub rotation: f32,
}

#[derive(Default, Debug, Clone, Copy)]
pub struct Output {
	pub linear_acceleration: Vector3<f32>,
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
