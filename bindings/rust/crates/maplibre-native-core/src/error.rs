use std::ffi::CStr;
use std::fmt;
use std::os::raw::c_char;

use maplibre_native_sys as sys;

pub type Result<T> = std::result::Result<T, Error>;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[non_exhaustive]
pub enum ErrorKind {
    InvalidArgument,
    InvalidState,
    WrongThread,
    Unsupported,
    NativeError,
    AbiVersionMismatch,
    UnknownStatus,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Error {
    kind: ErrorKind,
    raw_status: Option<i32>,
    diagnostic: String,
}

impl Error {
    pub fn new(kind: ErrorKind, raw_status: Option<i32>, diagnostic: impl Into<String>) -> Self {
        Self {
            kind,
            raw_status,
            diagnostic: diagnostic.into(),
        }
    }

    pub fn from_status(status: i32) -> Self {
        Self::from_status_and_diagnostic(status, capture_thread_diagnostic())
    }

    pub fn from_status_and_diagnostic(status: i32, diagnostic: impl Into<String>) -> Self {
        Self::new(kind_for_status(status), Some(status), diagnostic)
    }

    pub fn invalid_argument(diagnostic: impl Into<String>) -> Self {
        Self::new(ErrorKind::InvalidArgument, None, diagnostic)
    }

    pub fn abi_version_mismatch(expected: u32, actual: u32) -> Self {
        Self::new(
            ErrorKind::AbiVersionMismatch,
            None,
            format!("unsupported MapLibre Native C ABI version {actual}; expected {expected}"),
        )
    }

    pub fn kind(&self) -> ErrorKind {
        self.kind
    }

    pub fn raw_status(&self) -> Option<i32> {
        self.raw_status
    }

    pub fn diagnostic(&self) -> &str {
        &self.diagnostic
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self.raw_status {
            Some(status) => write!(f, "MapLibre Native status {status}: {}", self.diagnostic),
            None => f.write_str(&self.diagnostic),
        }
    }
}

impl std::error::Error for Error {}

pub fn check(status: i32) -> Result<()> {
    if status == sys::MLN_STATUS_OK {
        Ok(())
    } else {
        Err(Error::from_status(status))
    }
}

pub fn capture_thread_diagnostic() -> String {
    // SAFETY: The C API returns a thread-local, NUL-terminated string pointer
    // that is valid until the next diagnostic-writing C API call on this thread.
    // This helper copies it immediately and treats a defensive null as empty.
    unsafe { copy_c_string_lossy(sys::mln_thread_last_error_message()) }
}

pub fn kind_for_status(status: i32) -> ErrorKind {
    match status {
        sys::MLN_STATUS_INVALID_ARGUMENT => ErrorKind::InvalidArgument,
        sys::MLN_STATUS_INVALID_STATE => ErrorKind::InvalidState,
        sys::MLN_STATUS_WRONG_THREAD => ErrorKind::WrongThread,
        sys::MLN_STATUS_UNSUPPORTED => ErrorKind::Unsupported,
        sys::MLN_STATUS_NATIVE_ERROR => ErrorKind::NativeError,
        _ => ErrorKind::UnknownStatus,
    }
}

unsafe fn copy_c_string_lossy(ptr: *const c_char) -> String {
    if ptr.is_null() {
        return String::new();
    }

    // SAFETY: The caller promises that ptr is either null or points to a valid
    // NUL-terminated C string for the duration of this call.
    unsafe { CStr::from_ptr(ptr) }
        .to_string_lossy()
        .into_owned()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    // Spec coverage: BND-021.
    fn maps_unknown_status_without_losing_raw_status() {
        let error = Error::from_status_and_diagnostic(-123_456, "future status");

        assert_eq!(error.kind(), ErrorKind::UnknownStatus);
        assert_eq!(error.raw_status(), Some(-123_456));
        assert_eq!(error.diagnostic(), "future status");
    }

    #[test]
    fn invalid_native_calls_capture_status_and_diagnostic() {
        let error = check(unsafe { sys::mln_network_status_set(999_999) }).unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));
        assert!(error.diagnostic().contains("network status"));
    }

    #[test]
    fn diagnostic_is_copied_before_later_c_calls_replace_it() {
        let error = check(unsafe { sys::mln_network_status_set(999_999) }).unwrap_err();
        let copied = error.diagnostic().to_owned();
        let mut status = 0;

        check(unsafe { sys::mln_network_status_get(&mut status) }).unwrap();
        let current_diagnostic = capture_thread_diagnostic();

        assert_eq!(error.diagnostic(), copied);
        assert_ne!(current_diagnostic, copied);
    }
}
