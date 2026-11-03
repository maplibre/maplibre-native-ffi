// Generated from C headers by tools/bindgen. Do not edit.
/// The result of a call or of asynchronous work.
///
/// See `mln_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum Status {
    Ok,
    /// An argument is invalid, such as malformed JSON or a value out of range.
    InvalidArgument,
    /// The call doesn't fit the current state, such as removing a source that a
    /// layer still uses.
    InvalidState,
    /// The call came from a thread that can't make it.
    WrongThread,
    /// The platform, the build, or the render target lacks the feature.
    Unsupported,
    /// The library or the platform hit an error, such as a failed graphics
    /// driver call.
    NativeError,
    /// The work stopped before it finished, such as when its handle was
    /// released.
    Cancelled,
    /// Another call is still using the same object.
    Busy,
    /// The render target can't be drawn to anymore.
    TargetLost,
    /// Nothing has the given ID.
    NotFound,
    /// Something already has the given ID.
    AlreadyExists,
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
            -10 => Self::NotFound,
            -11 => Self::AlreadyExists,
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
            Self::NotFound => -10,
            Self::AlreadyExists => -11,
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
            Self::NotFound => f.write_str("NOT_FOUND"),
            Self::AlreadyExists => f.write_str("ALREADY_EXISTS"),
            Self::Unknown(value) => write!(f, "UNKNOWN({value})"),
        }
    }
}
