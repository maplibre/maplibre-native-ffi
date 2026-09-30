//! Helpers shared by this crate's integration tests.

use crate::{MapHandle, RuntimeHandle};

/// Releases an owner whose release completes asynchronously and blocks until
/// the native release finishes.
pub(crate) trait CloseAndWait {
    fn close_and_wait(self);
}

impl CloseAndWait for RuntimeHandle {
    fn close_and_wait(self) {
        crate::completion::blocking(self.release());
    }
}

impl CloseAndWait for MapHandle {
    fn close_and_wait(self) {
        crate::completion::blocking(self.release());
    }
}

/// Submits a runtime barrier and blocks until every runtime submission
/// accepted before it has reached a terminal disposition.
pub(crate) fn await_runtime_barrier(runtime: &RuntimeHandle) {
    crate::completion::blocking(runtime.barrier());
}

pub(crate) fn map_options(width: u32, height: u32, scale_factor: f64) -> crate::MapOptions {
    crate::MapOptions {
        initial_extent: crate::LogicalExtent::new(width, height, scale_factor),
        ..Default::default()
    }
}
