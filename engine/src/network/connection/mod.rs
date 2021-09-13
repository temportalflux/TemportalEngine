mod connection;
pub use connection::*;

mod list;
pub use list::*;

mod process_connect;
pub use process_connect::*;

mod process_disconnect;
pub use process_disconnect::*;

#[derive(Debug, Default, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Id(pub usize);

impl std::fmt::Display for Id {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		write!(f, "Id({})", self.0)
	}
}
