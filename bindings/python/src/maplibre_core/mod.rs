//! The ownership, callback, and memory runtime this extension used from the
//! core crate before the safe crate absorbed it. The extension moves onto the
//! safe crate's API in a later change.

pub mod callback;
pub mod handle;
pub mod memory;
pub mod ptr;
pub mod string;

use std::mem::MaybeUninit;

pub use maplibre_native_ffi::{Error, Result, Status};
use maplibre_native_ffi_sys as sys;

/// Fails when the linked library has another C ABI version.
pub fn validate_abi_version() -> Result<()> {
    // SAFETY: mln_c_version takes no arguments and returns the process-global C
    // ABI version for the linked native library.
    let actual = unsafe { sys::mln_c_version() };
    if actual == sys::MLN_C_VERSION {
        Ok(())
    } else {
        Err(Error::abi_version_mismatch(sys::MLN_C_VERSION, actual))
    }
}

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
    let message = unsafe { &raw const (*raw).message }.cast::<u8>();
    // SAFETY: all zeroes is a valid diagnostic, and only its length is read.
    let capacity = unsafe { std::mem::zeroed::<sys::mln_diagnostic>() }
        .message
        .len();
    let mut length = 0;
    // SAFETY: a null byte lies within the message capacity, and every byte
    // before it is initialized.
    while length < capacity && unsafe { message.add(length).read() } != 0 {
        length += 1;
    }
    // SAFETY: the first length bytes are initialized, as read above.
    let diagnostic =
        String::from_utf8_lossy(unsafe { std::slice::from_raw_parts(message, length) });
    Err(Error::from_status_and_diagnostic(status, diagnostic))
}
