pub use nalgebra;
pub mod ops;

pub fn interp_to(current: f32, target: f32, delta_time: f32, speed: f32) -> f32 {
	if speed <= 0.0 {
		return target;
	}
	let dist = target - current;
	if dist.powi(2) < f32::EPSILON {
		return target;
	}
	return current + (dist * (delta_time * speed).max(0.0).min(1.0));
}

pub fn map_to<TRange>(value: f32, range: TRange, step: f32) -> f32
where
	TRange: std::ops::RangeBounds<f32>,
{
	use std::ops::Bound::*;

	let mut value = value;

	while match range.start_bound() {
		Included(start) => value < *start,
		Excluded(start) => value <= *start,
		Unbounded => false,
	} {
		value += step;
	}

	while match range.end_bound() {
		Included(end) => value > *end,
		Excluded(end) => value >= *end,
		Unbounded => false,
	} {
		value -= step;
	}

	value
}
