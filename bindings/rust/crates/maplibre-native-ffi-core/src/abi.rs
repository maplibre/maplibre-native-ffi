use maplibre_native_ffi_sys as sys;

use crate::error::{Error, Result};

pub const EXPECTED_C_ABI_VERSION: u32 = 0;

/// Fails when the linked library has another C ABI version. Every entry point
/// that takes no handle calls this, or [`require_abi_version`], first.
pub fn validate_abi_version() -> Result<()> {
    // SAFETY: mln_c_version takes no arguments and returns the process-global C
    // ABI version for the linked native library.
    validate_abi_version_value(unsafe { sys::mln_c_version() })
}

/// Panics when the linked library has another C ABI version, for an entry
/// point with no error channel, such as a value's native default.
pub fn require_abi_version() {
    if let Err(error) = validate_abi_version() {
        panic!("{error}");
    }
}

fn validate_abi_version_value(actual: u32) -> Result<()> {
    if actual == EXPECTED_C_ABI_VERSION {
        Ok(())
    } else {
        Err(Error::abi_version_mismatch(EXPECTED_C_ABI_VERSION, actual))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::Status;

    #[test]
    fn another_c_abi_version_is_rejected_as_unsupported() {
        let error = validate_abi_version_value(EXPECTED_C_ABI_VERSION + 1).unwrap_err();
        assert_eq!(error.status(), Status::Unsupported);
        assert!(error.to_string().contains("C ABI version"));
        assert!(validate_abi_version().is_ok());
    }
}
