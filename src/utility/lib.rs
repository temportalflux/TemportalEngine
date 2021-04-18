#[path = "error.rs"]
mod error;
pub use error::*;

pub use temportal_graphics::utility::make_version;

pub fn for_each_valid_or_discard<T: ?Sized>(
	collection: &mut Vec<std::rc::Weak<T>>,
	callback: impl Fn(&T) -> crate::utility::Result<()>,
) -> crate::utility::Result<()> {
	let mut invalid_indices: Vec<usize> = Vec::new();
	for i in 0..collection.len() {
		match collection[i].upgrade() {
			Some(element) => callback(&element)?,
			None => invalid_indices.push(i),
		}
	}
	// Remove all invalid listeners (weak pointers pointing at nothing),
	// starting with those at the end of the listener list.
	while let Some(index) = invalid_indices.pop() {
		collection.swap_remove(index);
	}
	Ok(())
}
