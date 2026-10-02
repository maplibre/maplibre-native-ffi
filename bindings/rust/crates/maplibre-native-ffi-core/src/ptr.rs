use crate::error::Error;

/// The error for a handle argument that holds the null handle.
pub fn null_handle_error(name: &'static str) -> Error {
    Error::invalid_argument(format!("{name} must not be the null handle"))
}
