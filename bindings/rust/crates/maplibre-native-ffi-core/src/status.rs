// Generated from C headers by tools/bindgen. Do not edit.
/// Status values returned by status-returning functions.
///
/// See `mln_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum Status {
    Ok,
    /// A pointer, size field, mask, or handle argument was invalid.
    InvalidArgument,
    /// The object is valid but not currently in a state that permits the call.
    InvalidState,
    /// The handle is thread-affine and the call came from the wrong thread.
    WrongThread,
    /// The entry point or requested behavior is unavailable in this build.
    Unsupported,
    /// A native MapLibre error or C++ exception was converted to status.
    NativeError,
    /// The operation reached its terminal cancelled disposition.
    Cancelled,
    /// A conflicting driver call or lifecycle transition is in flight.
    Busy,
    /// The render target or graphics receiver was irreversibly lost.
    TargetLost,
    /// A nonblocking drain or acquisition has nothing to return yet.
    NotReady,
    /// A call named an ID with no live object behind it.
    NotFound,
    Unknown(i32),
}
impl Status {
    pub fn from_raw(raw: i32) -> Self {
        match raw {
            0 => Self::Ok,
            -1 => Self::InvalidArgument,
            -2 => Self::InvalidState,
            -3 => Self::WrongThread,
            -4 => Self::Unsupported,
            -5 => Self::NativeError,
            -6 => Self::Cancelled,
            -7 => Self::Busy,
            -8 => Self::TargetLost,
            -9 => Self::NotReady,
            -10 => Self::NotFound,
            value => Self::Unknown(value),
        }
    }
    pub fn as_raw(self) -> i32 {
        match self {
            Self::Ok => 0,
            Self::InvalidArgument => -1,
            Self::InvalidState => -2,
            Self::WrongThread => -3,
            Self::Unsupported => -4,
            Self::NativeError => -5,
            Self::Cancelled => -6,
            Self::Busy => -7,
            Self::TargetLost => -8,
            Self::NotReady => -9,
            Self::NotFound => -10,
            Self::Unknown(value) => value,
        }
    }
}
impl Default for Status {
    fn default() -> Self {
        Self::from_raw(0)
    }
}
impl std::fmt::Display for Status {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Ok => f.write_str("OK"),
            Self::InvalidArgument => f.write_str("INVALID_ARGUMENT"),
            Self::InvalidState => f.write_str("INVALID_STATE"),
            Self::WrongThread => f.write_str("WRONG_THREAD"),
            Self::Unsupported => f.write_str("UNSUPPORTED"),
            Self::NativeError => f.write_str("NATIVE_ERROR"),
            Self::Cancelled => f.write_str("CANCELLED"),
            Self::Busy => f.write_str("BUSY"),
            Self::TargetLost => f.write_str("TARGET_LOST"),
            Self::NotReady => f.write_str("NOT_READY"),
            Self::NotFound => f.write_str("NOT_FOUND"),
            Self::Unknown(value) => write!(f, "UNKNOWN({value})"),
        }
    }
}
