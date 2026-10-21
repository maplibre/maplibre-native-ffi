use maplibre_native_ffi_sys as sys;

use crate::error::{Error, Result};

pub const EXPECTED_C_ABI_VERSION: u32 = 0;

pub fn validate_abi_version() -> Result<()> {
    // SAFETY: mln_c_version takes no arguments and returns the process-global C
    // ABI version for the linked native library.
    validate_abi_version_value(unsafe { sys::mln_c_version() })
}

fn validate_abi_version_value(actual: u32) -> Result<()> {
    if actual == EXPECTED_C_ABI_VERSION {
        Ok(())
    } else {
        Err(Error::abi_version_mismatch(EXPECTED_C_ABI_VERSION, actual))
    }
}
