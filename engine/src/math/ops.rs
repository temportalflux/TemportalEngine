use crate::math::nalgebra::{Unit, UnitQuaternion, Vector2, Vector3};

pub fn look_at_3d(
	prev_forward: &Vector3<f32>,
	next_forward: &Vector3<f32>,
	up: &Unit<Vector3<f32>>,
) -> UnitQuaternion<f32> {
	let dot = prev_forward.dot(next_forward);
	//println!("{} dot {} = {}", prev_forward, next_forward, dot);
	match dot {
		alignment if (alignment + 1.0).abs() <= f32::EPSILON => {
			UnitQuaternion::from_axis_angle(up, 180_f32.to_radians())
		}
		alignment if (alignment - 1.0).abs() <= f32::EPSILON => UnitQuaternion::identity(),
		alignment => {
			//println!("{} {}", alignment, alignment.acos());
			let axis = prev_forward.cross(next_forward);
			UnitQuaternion::from_axis_angle(&Unit::new_normalize(axis), alignment.acos())
		}
	}
}

/// Projects a point onto a line segment, returning the distance from the point to the line segment.
/// Based on https://dev.to/thatkyleburke/generating-signed-distance-fields-from-truetype-fonts-calculating-the-distance-33io
/// https://dev-to-uploads.s3.amazonaws.com/i/dkggpi3kk1co139fxpey.png
pub fn distance_point_to_line_segment(
	point: Vector2<f32>,
	segment_start: Vector2<f32>,
	segment_end: Vector2<f32>,
) -> f32 {
	// A vector whose direction is the orientation from `line_start` to `line_end`,
	// and whose magnitude is the distance between start and end.
	let line = segment_end - segment_start;
	// The vector from `line_start` to `point`.
	let start_to_point = point - segment_start;

	let dot = start_to_point.dot(&line);
	// How far along the line the point `p` is projected to.
	let t = (dot / line.magnitude_squared()).max(0.0).min(1.0);

	// A vector whose direction matches `line`,
	// but whose magnitude is the distance from `start` to the spot `point` is projected to.
	let projection = (line * t) + segment_start;
	let point_to_projection = projection - point;

	point_to_projection.magnitude()
}

/// Returns true if the point is to the right of / has crossed a line.
/// Based on https://dev.to/thatkyleburke/generating-signed-distance-fields-from-truetype-fonts-calculating-the-sign-of-the-distance-6g6
pub fn has_crossed_line_segment(
	point: Vector2<f32>,
	segment_start: Vector2<f32>,
	segment_end: Vector2<f32>,
) -> bool {
	let line = segment_end - segment_start;
	// if the line is horizontal, then it is ignored
	if line.y == 0.0 {
		return false;
	}

	let t = (point - segment_start).y / line.y;
	let x = segment_start.x + t * line.x;

	let is_right_of_line = x > point.x;
	let between_endpoints = 0.0 < t && t < 1.0;
	let starts_upwards_line = t == 0.0 && line.y.is_sign_positive();
	let ends_downwards_line = t == 1.0 && line.y.is_sign_negative();

	is_right_of_line && (between_endpoints || starts_upwards_line || ends_downwards_line)
}

/// Returns the distance from a point to the closest point on a bezier curve.
/// Based on https://dev.to/thatkyleburke/generating-signed-distance-fields-from-truetype-fonts-calculating-the-distance-33io
pub fn distance_point_to_bezier(
	point: Vector2<f32>,
	start: Vector2<f32>,
	control: Vector2<f32>,
	end: Vector2<f32>,
) -> f32 {
	let start_to_control = control - start;
	let point_to_start = start - point;

	// Coefficients of the cubic polynomial
	let a = start - (control * 2.0) + end;
	let d4 = a.magnitude_squared();
	let d3 = a.dot(&start_to_control);
	let d2 = a.dot(&point_to_start) + (start_to_control.magnitude_squared() * 2.0);
	let d1 = start_to_control.dot(&point_to_start);
	let d0 = point_to_start.magnitude_squared();

	// Coefficients of the depressed cubic
	let dp = (d4 * d2 - 3.0 * d3 * d3) / d4.powi(2);
	let dq = (2.0 * d3.powi(3) - (d4 * d3 * d2) + d4.powi(2) * d1) / d4.powi(3);

	// Roots of the depressed cubic
	let discriminant = 4.0 * dp.powi(3) + 27.0 * dq.powi(2);
	let depressed_roots = if dp == 0.0 && dq == 0.0 {
		// 0 is the only solution
		vec![0.0]
	} else if discriminant > 0.0 {
		// only 1 solution, and can use Cardano's formula
		let a = -dq / 2.0;
		let b = (discriminant / 108.0).sqrt();
		vec![(a + b).cbrt() + (a - b).cbrt()]
	} else if discriminant < 0.0 {
		// there are 3 solutions, solvable via trigonometry
		let a = 2.0 * (-dp / 3.0).sqrt();
		let b = (1.0 / 3.0) * ((3.0 * dq) / (2.0 * dp) * (-3.0 / dp).sqrt()).acos();
		(0..3)
			.into_iter()
			.map(|k| a * (b - (2.0 * std::f32::consts::PI * (k as f32) / 3.0)).cos())
			.collect()
	} else {
		let a = 3.0 * dq;
		vec![a / dp, -a / (2.0 * dp)]
	};

	// finally, the minimum distance can be determined based on the roots
	let mut min_dist = f32::MAX;
	for root in depressed_roots {
		let t = (root - d3 / d4).max(0.0).min(1.0);
		let dist = ((d4 * t.powi(4))
			+ (4.0 * d3 * t.powi(3))
			+ (2.0 * d2 * t.powi(2))
			+ (4.0 * d1 * t)
			+ d0)
			.sqrt();
		min_dist = min_dist.min(dist);
	}

	min_dist
}

/// Returns the number of times a point-ray crosses a bezier curve.
/// Based on https://dev.to/thatkyleburke/generating-signed-distance-fields-from-truetype-fonts-calculating-the-sign-of-the-distance-6g6
pub fn count_intercepts_on_bezier(
	point: Vector2<f32>,
	start: Vector2<f32>,
	control: Vector2<f32>,
	end: Vector2<f32>,
) -> u32 {
	let u = start.y - 2.0 * control.y + end.y;

	if u == 0.0 {
		let line = end - start;
		let start_to_point = point - start;
		let t = start_to_point.y / line.y;
		let a = 1.0 - t;
		let x = (a.powi(2) * start.x) + (2.0 * a * t * control.x) + (t.powi(2) * end.x);

		let is_right_of_line = x > point.x;
		let between_endpoints = 0.0 < t && t < 1.0;
		let starts_upwards_line = t == 0.0 && line.y.is_sign_positive();
		let ends_downwards_line = t == 1.0 && line.y.is_sign_negative();

		return (is_right_of_line
			&& (between_endpoints || starts_upwards_line || ends_downwards_line)) as u32;
	}

	let w = (point.y * start.y) - (2.0 * point.y * control.y) + (point.y * end.y)
		- (start.y * end.y)
		+ control.y.powi(2);

	if w.is_sign_negative() {
		return 0;
	}

	let w = w.sqrt();
	let control_to_start = start - control;

	let intercept = |t: f32| -> f32 {
		let a = 1.0 - t;
		(a.powi(2) * start.x) + (2.0 * a * t * control.x) + (t.powi(2) * end.x)
	};

	let t1 = (control_to_start.y + w) / u;
	let x1 = intercept(t1);
	let t2 = (control_to_start.y - w) / u;
	let x2 = intercept(t2);
	let start_dir = if start.y == control.y {
		end - start
	} else {
		control - start
	};
	let end_dir = if end.y == control.y {
		end - start
	} else {
		end - control
	};

	if t1 == t2 {
		let is_right_of_line = x1 > point.x;
		let starts_upwards_line = t1 == 0.0 && start_dir.y.is_sign_positive();
		let ends_downwards_line = t1 == 1.0 && end_dir.y.is_sign_negative();
		(is_right_of_line && (starts_upwards_line || ends_downwards_line)) as u32
	} else {
		let is_crossing = |x: f32, t: f32| -> bool {
			let is_right_of_line = x > point.x;
			let between_endpoints = 0.0 < t && t < 1.0;
			let starts_upwards_line = t == 0.0 && start_dir.y.is_sign_positive();
			let ends_downwards_line = t == 1.0 && end_dir.y.is_sign_negative();
			is_right_of_line && (between_endpoints || starts_upwards_line || ends_downwards_line)
		};

		(is_crossing(x1, t1) as u32) + (is_crossing(x2, t2) as u32)
	}
}
