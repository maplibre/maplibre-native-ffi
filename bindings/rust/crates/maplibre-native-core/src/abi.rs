use maplibre_native_sys as sys;

use crate::error::{Error, Result};

pub const EXPECTED_C_ABI_VERSION: u32 = 0;

pub fn validate_abi_version() -> Result<()> {
    // SAFETY: mln_c_version takes no arguments and returns the process-global C
    // ABI version for the linked native library.
    validate_abi_version_value(unsafe { sys::mln_c_version() })
}

pub fn validate_abi_version_value(actual: u32) -> Result<()> {
    if actual == EXPECTED_C_ABI_VERSION {
        Ok(())
    } else {
        Err(Error::abi_version_mismatch(EXPECTED_C_ABI_VERSION, actual))
    }
}

#[cfg(test)]
mod tests {
    use crate::error::ErrorKind;

    use super::*;

    #[test]
    fn accepts_current_abi_version() {
        validate_abi_version().unwrap();
        validate_abi_version_value(EXPECTED_C_ABI_VERSION).unwrap();
    }

    #[test]
    fn rejects_unexpected_abi_version() {
        let error = validate_abi_version_value(EXPECTED_C_ABI_VERSION + 1).unwrap_err();

        assert_eq!(error.kind(), ErrorKind::AbiVersionMismatch);
        assert_eq!(error.raw_status(), None);
        assert!(
            error
                .diagnostic()
                .contains("unsupported MapLibre Native C ABI version")
        );
    }
}
