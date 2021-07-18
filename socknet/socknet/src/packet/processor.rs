use super::{AnyBox, Guarantee, Kind};
use crate::VoidResult;
use std::net::SocketAddr;

pub trait Processor<TKind>
where
	TKind: Kind + Sized + 'static,
{
	fn process_boxed(boxed: AnyBox, source: SocketAddr, guarantee: Guarantee) -> VoidResult {
		Self::process(&mut *boxed.downcast::<TKind>().unwrap(), source, guarantee)
	}

	fn process(data: &mut TKind, source: SocketAddr, guarantee: Guarantee) -> VoidResult;
}
