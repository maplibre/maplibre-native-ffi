// Generated from C headers by tools/bindgen. Do not edit.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[non_exhaustive]
pub enum ErrorKind {
    InvalidArgument,
    InvalidState,
    WrongThread,
    Unsupported,
    NativeError,
    Cancelled,
    Busy,
    TargetLost,
    NotReady,
    NotFound,
    AbiVersionMismatch,
    UnknownStatus,
}
fn kind_for_status(status: i32) -> ErrorKind {
    match status {
        sys::MLN_STATUS_INVALID_ARGUMENT => ErrorKind::InvalidArgument,
        sys::MLN_STATUS_INVALID_STATE => ErrorKind::InvalidState,
        sys::MLN_STATUS_WRONG_THREAD => ErrorKind::WrongThread,
        sys::MLN_STATUS_UNSUPPORTED => ErrorKind::Unsupported,
        sys::MLN_STATUS_NATIVE_ERROR => ErrorKind::NativeError,
        sys::MLN_STATUS_CANCELLED => ErrorKind::Cancelled,
        sys::MLN_STATUS_BUSY => ErrorKind::Busy,
        sys::MLN_STATUS_TARGET_LOST => ErrorKind::TargetLost,
        sys::MLN_STATUS_NOT_READY => ErrorKind::NotReady,
        sys::MLN_STATUS_NOT_FOUND => ErrorKind::NotFound,
        _ => ErrorKind::UnknownStatus,
    }
}
pub fn status_for_error(error: &Error) -> sys::mln_status {
    if let Some(status) = error.raw_status() {
        return status;
    }
    match error.kind() {
        ErrorKind::InvalidArgument => sys::MLN_STATUS_INVALID_ARGUMENT,
        ErrorKind::InvalidState => sys::MLN_STATUS_INVALID_STATE,
        ErrorKind::WrongThread => sys::MLN_STATUS_WRONG_THREAD,
        ErrorKind::Unsupported => sys::MLN_STATUS_UNSUPPORTED,
        ErrorKind::NativeError => sys::MLN_STATUS_NATIVE_ERROR,
        ErrorKind::Cancelled => sys::MLN_STATUS_CANCELLED,
        ErrorKind::Busy => sys::MLN_STATUS_BUSY,
        ErrorKind::TargetLost => sys::MLN_STATUS_TARGET_LOST,
        ErrorKind::NotReady => sys::MLN_STATUS_NOT_READY,
        ErrorKind::NotFound => sys::MLN_STATUS_NOT_FOUND,
        _ => sys::MLN_STATUS_NATIVE_ERROR,
    }
}
