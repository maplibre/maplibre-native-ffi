use maplibre_native_ffi_sys as sys;

use crate::error::{Error, Result};

pub const EXPECTED_C_ABI_VERSION: u32 = 0;

#[cfg(feature = "abi-version-override")]
thread_local! {
    // The initializer is const. Clippy reports it anyway on targets whose
    // thread locals the standard library emulates, such as Android.
    #[allow(clippy::missing_const_for_thread_local)]
    static VERSION_OVERRIDE: std::cell::Cell<Option<u32>> = const { std::cell::Cell::new(None) };
}

/// Makes the C ABI checks on the calling thread see `version` in place of the
/// loaded library's, or the library's again for `None`. For tests only.
#[cfg(feature = "abi-version-override")]
#[doc(hidden)]
pub fn set_abi_version_override(version: Option<u32>) {
    VERSION_OVERRIDE.set(version);
}

pub fn validate_abi_version() -> Result<()> {
    #[cfg(feature = "abi-version-override")]
    if let Some(version) = VERSION_OVERRIDE.get() {
        return validate_abi_version_value(version);
    }
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
