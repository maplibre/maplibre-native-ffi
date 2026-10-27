use std::fmt;
use std::mem::MaybeUninit;

use maplibre_native_ffi_sys as sys;

pub type Result<T> = std::result::Result<T, Error>;

use crate::Status;

/// A failure that native or this binding reports.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Error {
    /// A native status and the diagnostic that native copied for it.
    Status {
        status: Status,
        diagnostic: String,
        /// The map snapshot generation that a failed or cancelled map command
        /// published, and zero for any other failure.
        generation: u64,
    },
    AbiVersionMismatch {
        expected: u32,
        actual: u32,
    },
}

impl Error {
    pub fn new(status: Status, diagnostic: impl Into<String>) -> Self {
        Self::Status {
            status,
            diagnostic: diagnostic.into(),
            generation: 0,
        }
    }

    pub fn from_status_and_diagnostic(status: i32, diagnostic: impl Into<String>) -> Self {
        Self::new(Status::from_raw(status), diagnostic)
    }

    pub fn invalid_argument(diagnostic: impl Into<String>) -> Self {
        Self::new(Status::InvalidArgument, diagnostic)
    }

    pub fn abi_version_mismatch(expected: u32, actual: u32) -> Self {
        Self::AbiVersionMismatch { expected, actual }
    }

    pub fn status(&self) -> Status {
        match self {
            Self::Status { status, .. } => *status,
            Self::AbiVersionMismatch { .. } => Status::Unsupported,
        }
    }

    pub fn diagnostic(&self) -> &str {
        match self {
            Self::Status { diagnostic, .. } => diagnostic,
            Self::AbiVersionMismatch { .. } => "C ABI version mismatch",
        }
    }

    /// The map snapshot generation that a failed or cancelled map command
    /// published, and zero for any other failure.
    pub fn generation(&self) -> u64 {
        match self {
            Self::Status { generation, .. } => *generation,
            Self::AbiVersionMismatch { .. } => 0,
        }
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Status {
                status, diagnostic, ..
            } => write!(f, "{status}: {diagnostic}"),
            Self::AbiVersionMismatch { expected, actual } => write!(
                f,
                "unsupported MapLibre Native C ABI version {actual}; expected {expected}"
            ),
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

/// The capacity of a diagnostic's message, including its null byte, which the
/// generated record declares.
pub const DIAGNOSTIC_MESSAGE_CAPACITY: usize = {
    // SAFETY: all zeroes is a valid diagnostic, and only its length is read.
    let diagnostic: sys::mln_diagnostic = unsafe { std::mem::zeroed() };
    diagnostic.message.len()
};

/// Copies a diagnostic's message, which is initialized through its null byte.
unsafe fn diagnostic_message(diagnostic: *const sys::mln_diagnostic) -> String {
    // SAFETY: the caller promises the diagnostic outlives this call.
    let message = unsafe { &raw const (*diagnostic).message }.cast::<u8>();
    let capacity = DIAGNOSTIC_MESSAGE_CAPACITY;
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

    /// Runs a call that fails with `status` after writing `message` into its
    /// diagnostic, the way a native call reports a failure.
    fn failing_call(status: sys::mln_status, message: &[u8]) -> Error {
        check(|diagnostic| {
            // SAFETY: check hands the call one diagnostic, and every message
            // written here fits its capacity.
            unsafe {
                let out = (&raw mut (*diagnostic).message).cast::<u8>();
                std::ptr::copy_nonoverlapping(message.as_ptr(), out, message.len());
            }
            status
        })
        .unwrap_err()
    }

    #[test]
    fn a_failed_call_maps_its_status_and_keeps_an_unknown_code() {
        let statuses = [
            (sys::MLN_STATUS_INVALID_ARGUMENT, Status::InvalidArgument),
            (sys::MLN_STATUS_INVALID_STATE, Status::InvalidState),
            (sys::MLN_STATUS_WRONG_THREAD, Status::WrongThread),
            (sys::MLN_STATUS_UNSUPPORTED, Status::Unsupported),
            (sys::MLN_STATUS_CANCELLED, Status::Cancelled),
            (sys::MLN_STATUS_BUSY, Status::Busy),
            (sys::MLN_STATUS_TARGET_LOST, Status::TargetLost),
            (sys::MLN_STATUS_NOT_READY, Status::NotReady),
            (sys::MLN_STATUS_NOT_FOUND, Status::NotFound),
            (sys::MLN_STATUS_NATIVE_ERROR, Status::NativeError),
        ];
        for (status, expected) in statuses {
            let error = failing_call(status, b"failed\0");
            assert_eq!(error.status(), expected);
            assert_eq!(error.to_string(), format!("{expected}: failed"));
            assert_eq!(error.status().as_raw(), status);
            assert_eq!(error.diagnostic(), "failed");
        }

        // A status this binding predates keeps its code rather than folding
        // into a known one.
        let error = failing_call(-123_456, b"future status\0");
        assert_eq!(error.status(), Status::Unknown(-123_456));
        assert_eq!(error.status().as_raw(), -123_456);
        assert_eq!(error.diagnostic(), "future status");
        assert_eq!(error.to_string(), "UNKNOWN(-123456): future status");
    }

    #[test]
    fn a_diagnostic_is_bounded_by_its_capacity_and_empty_when_none_was_written() {
        // A message that fills the buffer with no terminator ends at capacity.
        let capacity = DIAGNOSTIC_MESSAGE_CAPACITY;
        let error = failing_call(sys::MLN_STATUS_INVALID_STATE, &vec![b'x'; capacity]);
        assert_eq!(error.diagnostic(), "x".repeat(capacity));

        // Bytes that are not UTF-8 are replaced, not dropped.
        let error = failing_call(sys::MLN_STATUS_INVALID_STATE, b"bad \xff byte\0");
        assert_eq!(error.diagnostic(), "bad \u{fffd} byte");

        // Each call starts from an empty message, so a call that fails without
        // writing one carries none, not the previous call's.
        let error = check(|_| sys::MLN_STATUS_INVALID_ARGUMENT).unwrap_err();
        assert_eq!(error.status(), Status::InvalidArgument);
        assert_eq!(error.diagnostic(), "");
    }
}
