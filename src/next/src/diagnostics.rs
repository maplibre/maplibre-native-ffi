// Ports maplibre-native-ffi src/diagnostics/diagnostics.cpp (write_diagnostic)
// and src/c_api/boundary.hpp (native_boundary): every entry point writes its
// failure message into the caller's mln_diagnostic, or an empty message on
// success, and no Rust panic crosses the C boundary.

use crate::types::*;
use std::panic::{AssertUnwindSafe, catch_unwind};

/// A failed call: its status and message.
pub struct Failure {
    pub status: i32,
    pub message: String,
}

impl Failure {
    pub fn new(status: i32, message: impl Into<String>) -> Failure {
        Failure { status, message: message.into() }
    }

    pub fn invalid_argument(message: impl Into<String>) -> Failure {
        Failure::new(MLN_STATUS_INVALID_ARGUMENT, message)
    }

    pub fn invalid_state(message: impl Into<String>) -> Failure {
        Failure::new(MLN_STATUS_INVALID_STATE, message)
    }
}

pub type CallResult = Result<(), Failure>;

/// The longest prefix of `message` that fits `capacity` bytes with a NUL,
/// cut at a UTF-8 character boundary (C++ `truncated`).
fn truncated(message: &str, capacity: usize) -> &str {
    if message.len() < capacity {
        return message;
    }
    let mut length = capacity.saturating_sub(1);
    while length > 0 && !message.is_char_boundary(length) {
        length -= 1;
    }
    &message[..length]
}

/// `write_diagnostic`.
///
/// # Safety
/// `diagnostic` is null or points at a writable `mln_diagnostic` whose
/// `size` says how many of its bytes are writable.
pub unsafe fn write(diagnostic: *mut MlnDiagnostic, message: &str) {
    if diagnostic.is_null() {
        return;
    }
    let message_offset = std::mem::offset_of!(MlnDiagnostic, message);
    // SAFETY: the caller's contract.
    let size = unsafe { (*diagnostic).size } as usize;
    if size <= message_offset {
        return;
    }
    let capacity = (size - message_offset).min(MLN_DIAGNOSTIC_MESSAGE_CAPACITY);
    let text = truncated(message, capacity);
    // SAFETY: `text.len() + 1 <= capacity` bytes of `message` are writable.
    unsafe {
        let out = (*diagnostic).message.as_mut_ptr().cast::<u8>();
        std::ptr::copy_nonoverlapping(text.as_ptr(), out, text.len());
        *out.add(text.len()) = 0;
    }
}

/// Runs an entry point's body: catches panics, writes the diagnostic and
/// returns the status (C++ `native_boundary`).
///
/// # Safety
/// As for `write`.
pub unsafe fn boundary(diagnostic: *mut MlnDiagnostic, body: impl FnOnce() -> CallResult) -> i32 {
    let outcome = match catch_unwind(AssertUnwindSafe(body)) {
        Ok(result) => result,
        Err(panic) => Err(Failure::new(MLN_STATUS_NATIVE_ERROR, panic_message(&panic))),
    };
    match outcome {
        Ok(()) => {
            // SAFETY: the caller's contract.
            unsafe { write(diagnostic, "") };
            MLN_STATUS_OK
        }
        Err(failure) => {
            // SAFETY: the caller's contract.
            unsafe { write(diagnostic, &failure.message) };
            failure.status
        }
    }
}

pub fn panic_message(panic: &Box<dyn std::any::Any + Send>) -> String {
    if let Some(message) = panic.downcast_ref::<&str>() {
        format!("native panic: {message}")
    } else if let Some(message) = panic.downcast_ref::<String>() {
        format!("native panic: {message}")
    } else {
        "native panic".to_owned()
    }
}

/// The stubs' result (stubs.rs).
///
/// # Safety
/// As for `write`.
pub unsafe fn unsupported(diagnostic: *mut MlnDiagnostic, name: &str) -> i32 {
    // SAFETY: the caller's contract.
    unsafe { write(diagnostic, &format!("{name} is not implemented by maplibre-native-next yet")) };
    MLN_STATUS_UNSUPPORTED
}

#[allow(dead_code)] // stubs.rs uses it when a stub without a diagnostic returns a status.
pub fn unsupported_status(_name: &str) -> i32 {
    MLN_STATUS_UNSUPPORTED
}
