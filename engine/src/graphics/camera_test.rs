use crate::{
	graphics::camera::Projection,
	math::nalgebra::{Isometry3, Matrix4, Point3, Unit, UnitQuaternion, Vector3},
	world,
};
use approx::assert_relative_eq;

// https://gamedev.stackexchange.com/questions/178643/the-view-matrix-finally-explained
/*
														-z (world forward)

																front:	-z
																back:		+z
																right:	+x
																left:		-x
																		^

(world left) -x					<						+							>					+x (world right)
										front:	-x							front:	+x
										back:		+x							back:		-x
										right:	-z							right:	+z
										left:		+z							left:		-z
																		v
																front:	+z
																back:		-z
																right:	-x
																left:		+x

														+z (world backward)
*/

fn global_front() -> Unit<Vector3<f32>> {
	world::global_forward()
}
fn global_back() -> Unit<Vector3<f32>> {
	-world::global_forward()
}
fn global_right() -> Unit<Vector3<f32>> {
	world::global_right()
}
fn global_left() -> Unit<Vector3<f32>> {
	-world::global_right()
}

fn make_look_at(
	orientation: &UnitQuaternion<f32>,
	expected_direction: &Unit<Vector3<f32>>,
) -> (Matrix4<f32>, Isometry3<f32>) {
	let position = Point3::new(0.0, 0.0, 0.0);
	let forward = orientation * global_front();
	assert_relative_eq!(forward, expected_direction);
	let up = orientation * world::global_up();
	let target = position + forward.into_inner();
	let target_expect = Point3::new(0.0, 0.0, 0.0) + expected_direction.into_inner();
	assert_relative_eq!(target, target_expect);
	(
		Matrix4::look_at_rh(&position, &target, &up),
		Isometry3::look_at_rh(&position, &target, &up),
	)
}

fn north() -> (Matrix4<f32>, Isometry3<f32>) {
	// Rotating 0 rad (0 deg), is equivalent to looking forwards (in world space), which is -Z
	let orientation = UnitQuaternion::from_axis_angle(&world::global_up(), 0.0);
	make_look_at(&orientation, &global_front())
}

fn south() -> (Matrix4<f32>, Isometry3<f32>) {
	// Rotating PI rad (180 deg), is equivalent to looking backwards (in world space), which is +Z
	let orientation = UnitQuaternion::from_axis_angle(&world::global_up(), std::f32::consts::PI);
	make_look_at(&orientation, &global_back())
}

fn west() -> (Matrix4<f32>, Isometry3<f32>) {
	// Rotating PI/2 rad (90 deg), is equivalent to looking west (left in world space), which is -X
	let orientation =
		UnitQuaternion::from_axis_angle(&world::global_up(), std::f32::consts::FRAC_PI_2);
	make_look_at(&orientation, &global_left())
}

fn east() -> (Matrix4<f32>, Isometry3<f32>) {
	// Rotating -PI/2 rad (-90 deg), is equivalent to looking east (right in world space), which is +X
	let orientation =
		UnitQuaternion::from_axis_angle(&world::global_up(), -std::f32::consts::FRAC_PI_2);
	make_look_at(&orientation, &global_right())
}

// Facing: North/World-Front
#[test]
fn north_matrix() {
	let (view, _) = north();

	#[rustfmt::skip]
	assert_relative_eq!(view, Matrix4::new(
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0,
	));

	// global front becomes local front
	assert_relative_eq!(
		view.transform_vector(&global_front()),
		global_front(),
		epsilon = 0.0001
	);
	// global back becomes local back
	assert_relative_eq!(
		view.transform_vector(&global_back()),
		global_back(),
		epsilon = 0.0001
	);
	// global right becomes local right
	assert_relative_eq!(
		view.transform_vector(&global_right()),
		global_right(),
		epsilon = 0.0001
	);
	// global left becomes local left
	assert_relative_eq!(
		view.transform_vector(&global_left()),
		global_left(),
		epsilon = 0.0001
	);
}

// Facing: North/World-Front
#[test]
fn north_iso() {
	let (_, iso) = north();

	// global front becomes local front
	assert_relative_eq!(
		iso.transform_vector(&global_front()),
		global_front(),
		epsilon = 0.0001
	);
	// global back becomes local back
	assert_relative_eq!(
		iso.transform_vector(&global_back()),
		global_back(),
		epsilon = 0.0001
	);
	// global right becomes local right
	assert_relative_eq!(
		iso.transform_vector(&global_right()),
		global_right(),
		epsilon = 0.0001
	);
	// global left becomes local left
	assert_relative_eq!(
		iso.transform_vector(&global_left()),
		global_left(),
		epsilon = 0.0001
	);
}

// Facing: South/World-Back
#[test]
fn south_matrix() {
	let (view, _) = south();

	#[rustfmt::skip]
	assert_relative_eq!(view, Matrix4::new(
		-1.0, 0.0,  0.0, 0.0,
		 0.0, 1.0,  0.0, 0.0,
		 0.0, 0.0, -1.0, 0.0,
		 0.0, 0.0,  0.0, 1.0,
	));

	// global front becomes local back
	assert_relative_eq!(
		view.transform_vector(&global_front()),
		global_back(),
		epsilon = 0.0001
	);
	// global back becomes local front
	assert_relative_eq!(
		view.transform_vector(&global_back()),
		global_front(),
		epsilon = 0.0001
	);
	// global right becomes local left
	assert_relative_eq!(
		view.transform_vector(&global_right()),
		global_left(),
		epsilon = 0.0001
	);
	// global left becomes local right
	assert_relative_eq!(
		view.transform_vector(&global_left()),
		global_right(),
		epsilon = 0.0001
	);
}

// Facing: South/World-Back
#[test]
fn south_iso() {
	let (_, iso) = south();

	// global front becomes local back
	assert_relative_eq!(
		iso.transform_vector(&global_front()),
		global_back(),
		epsilon = 0.0001
	);
	// global back becomes local front
	assert_relative_eq!(
		iso.transform_vector(&global_back()),
		global_front(),
		epsilon = 0.0001
	);
	// global right becomes local left
	assert_relative_eq!(
		iso.transform_vector(&global_right()),
		global_left(),
		epsilon = 0.0001
	);
	// global left becomes local right
	assert_relative_eq!(
		iso.transform_vector(&global_left()),
		global_right(),
		epsilon = 0.0001
	);
}

// Facing: East/World-Right
#[test]
fn east_matrix() {
	let (view, _) = east();

	#[rustfmt::skip]
	assert_relative_eq!(view, Matrix4::new(
		 0.0, 0.0, 1.0, 0.0,
		 0.0, 1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0, 0.0,
		 0.0, 0.0, 0.0, 1.0,
	));

	// global front becomes local left
	assert_relative_eq!(
		view.transform_vector(&global_front()),
		global_left(),
		epsilon = 0.0001
	);
	// global back becomes local right
	assert_relative_eq!(
		view.transform_vector(&global_back()),
		global_right(),
		epsilon = 0.0001
	);
	// global right becomes local front
	assert_relative_eq!(
		view.transform_vector(&global_right()),
		global_front(),
		epsilon = 0.0001
	);
	// global left becomes local back
	assert_relative_eq!(
		view.transform_vector(&global_left()),
		global_back(),
		epsilon = 0.0001
	);
}

// Facing: East/World-Right
#[test]
fn east_iso() {
	let (_, iso) = east();

	// global front becomes local left
	assert_relative_eq!(
		iso.transform_vector(&global_front()),
		global_left(),
		epsilon = 0.0001
	);
	// global back becomes local right
	assert_relative_eq!(
		iso.transform_vector(&global_back()),
		global_right(),
		epsilon = 0.0001
	);
	// global right becomes local front
	assert_relative_eq!(
		iso.transform_vector(&global_right()),
		global_front(),
		epsilon = 0.0001
	);
	// global left becomes local back
	assert_relative_eq!(
		iso.transform_vector(&global_left()),
		global_back(),
		epsilon = 0.0001
	);
}

// Facing: West/World-Left
#[test]
fn west_matrix() {
	let (view, _) = west();

	#[rustfmt::skip]
	assert_relative_eq!(view, Matrix4::new(
		0.0, 0.0, -1.0, 0.0,
		0.0, 1.0,  0.0, 0.0,
		1.0, 0.0,  0.0, 0.0,
		0.0, 0.0,  0.0, 1.0,
	));

	// global front becomes local right
	assert_relative_eq!(
		view.transform_vector(&global_front()),
		global_right(),
		epsilon = 0.0001
	);
	// global back becomes local left
	assert_relative_eq!(
		view.transform_vector(&global_back()),
		global_left(),
		epsilon = 0.0001
	);
	// global right becomes local back
	assert_relative_eq!(
		view.transform_vector(&global_right()),
		global_back(),
		epsilon = 0.0001
	);
	// global left becomes local front
	assert_relative_eq!(
		view.transform_vector(&global_left()),
		global_front(),
		epsilon = 0.0001
	);
}

// Facing: West/World-Left
#[test]
fn west_iso() {
	let (_, iso) = west();

	// global front becomes local right
	assert_relative_eq!(
		iso.transform_vector(&global_front()),
		global_right(),
		epsilon = 0.0001
	);
	// global back becomes local left
	assert_relative_eq!(
		iso.transform_vector(&global_back()),
		global_left(),
		epsilon = 0.0001
	);
	// global right becomes local back
	assert_relative_eq!(
		iso.transform_vector(&global_right()),
		global_back(),
		epsilon = 0.0001
	);
	// global left becomes local front
	assert_relative_eq!(
		iso.transform_vector(&global_left()),
		global_front(),
		epsilon = 0.0001
	);
}

#[allow(dead_code)]
fn camera_perspective() {
	let aspect_ratio = 1920.0 / 1080.0;
	let vertical_fov = 45.0;
	let z_near = 0.1;
	let z_far = 1000.0;
	let clip = Projection::perspective_right_hand_depth_zero_to_one(
		vertical_fov,
		aspect_ratio,
		z_near,
		z_far,
	);
	let clip_nalg =
		nalgebra::geometry::Perspective3::new(aspect_ratio, vertical_fov, z_near, z_far);
	assert_relative_eq!(clip, clip_nalg.as_matrix());
}
