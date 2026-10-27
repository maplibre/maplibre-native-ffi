use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::str;

use maplibre_native_ffi_sys as sys;

use super::{Error, Result};

pub fn c_string(value: &str) -> Result<CString> {
    CString::new(value).map_err(|_| embedded_nul_error())
}

pub fn buffer_view(value: &[u8]) -> sys::mln_buffer_view {
    sys::mln_buffer_view {
        data: if value.is_empty() {
            std::ptr::null()
        } else {
            value.as_ptr().cast()
        },
        size: value.len(),
    }
}

/// Copies a borrowed native string view into an owned Rust string.
///
/// # Safety
///
/// When `view.size` is nonzero, `view.data` must point to `view.size` bytes of
/// storage valid for the duration of this call.
pub unsafe fn copy_string_view(view: sys::mln_buffer_view) -> Result<String> {
    // SAFETY: the caller's contract, which this call needs only while it copies.
    unsafe { borrow_string_view(view) }.map(str::to_owned)
}

/// Borrows a native string view as text.
///
/// # Safety
///
/// When `view.size` is nonzero, `view.data` must point to `view.size` bytes of
/// storage valid for `'a`.
pub unsafe fn borrow_string_view<'a>(view: sys::mln_buffer_view) -> Result<&'a str> {
    if view.size == 0 {
        return Ok("");
    }
    if view.data.is_null() {
        return Err(Error::invalid_argument(
            "native string view data must not be null when size is nonzero",
        ));
    }
    // SAFETY: The caller promised the view points to view.size valid bytes.
    let bytes = unsafe { std::slice::from_raw_parts(view.data.cast::<u8>(), view.size) };
    str::from_utf8(bytes).map_err(|error| {
        Error::invalid_argument(format!("native string view was not valid UTF-8: {error}"))
    })
}

/// Copies a borrowed native string view into owned bytes.
///
/// # Safety
///
/// When `view.size` is nonzero, `view.data` must point to `view.size` bytes of
/// storage valid for the duration of this call.
pub unsafe fn copy_string_view_bytes(view: sys::mln_buffer_view) -> Result<Vec<u8>> {
    if view.size == 0 {
        return Ok(Vec::new());
    }
    if view.data.is_null() {
        return Err(Error::invalid_argument(
            "native string view data must not be null when size is nonzero",
        ));
    }

    // SAFETY: The caller promised the view points to view.size valid bytes.
    let bytes = unsafe { std::slice::from_raw_parts(view.data.cast::<u8>(), view.size) };
    Ok(bytes.to_vec())
}

/// Copies a borrowed NUL-terminated native string.
///
/// # Safety
///
/// `ptr` must be null or point to a valid NUL-terminated C string for the
/// duration of this call.
pub unsafe fn copy_c_string(ptr: *const c_char) -> Result<String> {
    // SAFETY: the caller's contract, which this call needs only while it copies.
    unsafe { borrow_c_string(ptr) }.map(str::to_owned)
}

/// Borrows a NUL-terminated native string as text, empty when `ptr` is null.
///
/// # Safety
///
/// `ptr` must be null or point to a valid NUL-terminated C string for `'a`.
pub unsafe fn borrow_c_string<'a>(ptr: *const c_char) -> Result<&'a str> {
    if ptr.is_null() {
        return Ok("");
    }
    // SAFETY: The caller promised ptr is a valid NUL-terminated C string.
    let bytes = unsafe { CStr::from_ptr(ptr) }.to_bytes();
    str::from_utf8(bytes)
        .map_err(|error| Error::invalid_argument(format!("native C string was not UTF-8: {error}")))
}

fn embedded_nul_error() -> Error {
    Error::invalid_argument("C string inputs must not contain embedded NUL characters")
}
