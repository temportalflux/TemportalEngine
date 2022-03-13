mod lib;
pub use lib::*;

fn main() {
	engine::run(Runtime::new())
}
