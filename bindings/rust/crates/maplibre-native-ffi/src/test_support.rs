//! Helpers shared by this crate's integration tests.

use crate::RuntimeHandle;

/// Submits a runtime barrier and blocks until every runtime submission
/// accepted before it has reached a terminal disposition.
pub(crate) fn await_runtime_barrier(runtime: &RuntimeHandle) {
    crate::completion::blocking(runtime.barrier());
}
