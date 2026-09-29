use std::fmt;
use std::mem::MaybeUninit;

use maplibre_native_ffi_sys as sys;

pub type Result<T> = std::result::Result<T, Error>;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[non_exhaustive]
pub enum ErrorKind {
    InvalidArgument,
    InvalidState,
    WrongThread,
    Unsupported,
    Cancelled,
    Busy,
    TargetLost,
    NotReady,
    NotFound,
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

/// Calls a status-returning C function with a diagnostic for its final
/// `out_diagnostic` parameter, and converts a failed status and that diagnostic
/// into an error.
pub fn check(call: impl FnOnce(*mut sys::mln_diagnostic) -> sys::mln_status) -> Result<()> {
    let mut diagnostic = MaybeUninit::<sys::mln_diagnostic>::uninit();
    let raw = diagnostic.as_mut_ptr();
    // SAFETY: raw points to storage for one mln_diagnostic. Native writes the
    // message, so only the size and an empty message are initialized here;
    // the empty message covers a call that fails without reaching native.
    unsafe {
        (&raw mut (*raw).size).write(std::mem::size_of::<sys::mln_diagnostic>() as u32);
        (&raw mut (*raw).message).cast::<u8>().write(0);
    }
    let status = call(raw);
    if status == sys::MLN_STATUS_OK {
        return Ok(());
    }
    // SAFETY: the message is null-terminated, either by native or above.
    Err(Error::from_status_and_diagnostic(status, unsafe {
        diagnostic_message(raw)
    }))
}

pub fn kind_for_status(status: i32) -> ErrorKind {
    match status {
        sys::MLN_STATUS_INVALID_ARGUMENT => ErrorKind::InvalidArgument,
        sys::MLN_STATUS_INVALID_STATE => ErrorKind::InvalidState,
        sys::MLN_STATUS_WRONG_THREAD => ErrorKind::WrongThread,
        sys::MLN_STATUS_UNSUPPORTED => ErrorKind::Unsupported,
        sys::MLN_STATUS_CANCELLED => ErrorKind::Cancelled,
        sys::MLN_STATUS_BUSY => ErrorKind::Busy,
        sys::MLN_STATUS_TARGET_LOST => ErrorKind::TargetLost,
        sys::MLN_STATUS_NOT_READY => ErrorKind::NotReady,
        sys::MLN_STATUS_NOT_FOUND => ErrorKind::NotFound,
        sys::MLN_STATUS_NATIVE_ERROR => ErrorKind::NativeError,
        _ => ErrorKind::UnknownStatus,
    }
}

/// Copies a diagnostic's message, which is initialized through its null byte.
unsafe fn diagnostic_message(diagnostic: *const sys::mln_diagnostic) -> String {
    // SAFETY: the caller promises the diagnostic outlives this call.
    let message = unsafe { &raw const (*diagnostic).message }.cast::<u8>();
    let capacity = sys::MLN_DIAGNOSTIC_MESSAGE_CAPACITY as usize;
    let mut length = 0;
    // SAFETY: the caller promises a null byte within the message capacity, and
    // every byte before it is initialized.
    while length < capacity && unsafe { message.add(length).read() } != 0 {
        length += 1;
    }
    // SAFETY: the first length bytes are initialized, as read above.
    String::from_utf8_lossy(unsafe { std::slice::from_raw_parts(message, length) }).into_owned()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn maps_unknown_status_without_losing_raw_status() {
        let error = Error::from_status_and_diagnostic(-123_456, "future status");

        assert_eq!(error.kind(), ErrorKind::UnknownStatus);
        assert_eq!(error.raw_status(), Some(-123_456));
        assert_eq!(error.diagnostic(), "future status");
    }

    #[test]
    fn invalid_native_calls_capture_status_and_diagnostic() {
        let error = check(|diagnostic| unsafe { sys::mln_network_status_set(999_999, diagnostic) })
            .unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));
        assert!(error.diagnostic().contains("network status"));
    }
}
