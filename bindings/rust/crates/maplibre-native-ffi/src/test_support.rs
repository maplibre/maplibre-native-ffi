//! Helpers shared by this crate's integration tests.

use crate::RuntimeHandle;

/// Submits a runtime barrier and blocks until every runtime submission
/// accepted before it has reached a terminal disposition.
pub(crate) fn await_runtime_barrier(runtime: &RuntimeHandle) {
    crate::completion::blocking(runtime.barrier());
}

pub(crate) fn ok_response(bytes: impl Into<Vec<u8>>) -> crate::ResourceResponse {
    crate::ResourceResponse {
        status: crate::ResourceResponseStatus::Ok,
        bytes: bytes.into(),
        ..Default::default()
    }
}
pub(crate) fn error_response(
    reason: crate::ResourceErrorReason,
    message: impl Into<String>,
) -> crate::ResourceResponse {
    crate::ResourceResponse {
        status: crate::ResourceResponseStatus::Error,
        error_reason: reason,
        error_message: Some(message.into()),
        ..Default::default()
    }
}

pub(crate) fn map_options(width: u32, height: u32, scale_factor: f64) -> crate::MapOptions {
    crate::MapOptions {
        initial_extent: crate::LogicalExtent::new(width, height, scale_factor),
        ..Default::default()
    }
}
