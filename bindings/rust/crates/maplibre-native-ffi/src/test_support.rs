//! Helpers shared by this crate's integration tests.

use crate::{MapHandle, Result, RuntimeHandle};

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

/// Creates a runtime after checking `actual_abi_version` instead of the
/// loaded library's version.
pub(crate) fn runtime_create_after_abi_version_check(
    actual_abi_version: u32,
) -> Result<RuntimeHandle> {
    maplibre_native_ffi_core::validate_abi_version_value(actual_abi_version)?;
    crate::runtime_create(&crate::RuntimeOptions::default())
}

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
