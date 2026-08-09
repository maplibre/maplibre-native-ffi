use std::ffi::{CStr, CString};
use std::marker::PhantomData;
use std::os::raw::c_char;
use std::ptr;
use std::str;

use maplibre_native_ffi_sys as sys;

use crate::error::{Error, Result};
use crate::handle::NativeHandle;

#[derive(Debug, Clone, Copy)]
pub struct StringView<'a> {
    raw: sys::mln_buffer_view,
    _lifetime: PhantomData<&'a str>,
}

impl<'a> StringView<'a> {
    pub fn new(value: &'a str) -> Self {
        let bytes = value.as_bytes();
        let data = if bytes.is_empty() {
            ptr::null()
        } else {
            bytes.as_ptr().cast()
        };
        Self {
            raw: sys::mln_buffer_view {
                data,
                size: bytes.len(),
            },
            _lifetime: PhantomData,
        }
    }

    pub fn raw(&self) -> sys::mln_buffer_view {
        self.raw
    }
}

impl<'a> From<&'a str> for StringView<'a> {
    fn from(value: &'a str) -> Self {
        Self::new(value)
    }
}

pub fn c_string(value: &str) -> Result<CString> {
    CString::new(value).map_err(|_| embedded_nul_error())
}

pub fn optional_c_string(value: Option<&str>) -> Result<Option<CString>> {
    value.map(c_string).transpose()
}

pub fn string_view(value: &str) -> StringView<'_> {
    StringView::new(value)
}

pub fn buffer_view(value: &[u8]) -> sys::mln_buffer_view {
    sys::mln_buffer_view {
        data: if value.is_empty() {
            ptr::null()
        } else {
            value.as_ptr().cast()
        },
        size: value.len(),
    }
}

/// Copies and releases an owned native buffer.
///
/// # Safety
///
/// `buffer` must be null or an owned buffer returned by the C API.
pub unsafe fn copy_owned_buffer(buffer: sys::mln_buffer) -> Result<Vec<u8>> {
    if buffer.to_raw() == 0 {
        return Ok(Vec::new());
    }
    // SAFETY: The caller transfers ownership of the live buffer to this guard.
    let buffer = unsafe { crate::handle::buffer(buffer) }?;
    let mut view = sys::mln_buffer_view {
        data: ptr::null(),
        size: 0,
    };
    // SAFETY: buffer is live and view points to writable storage.
    crate::check(unsafe { sys::mln_buffer_get(buffer.handle(), &mut view) })?;
    // SAFETY: The returned view remains valid while the guard is live.
    unsafe { copy_string_view_bytes(view) }
}

/// Copies a borrowed native string view into an owned Rust string.
///
/// # Safety
///
/// When `view.size` is nonzero, `view.data` must point to `view.size` bytes of
/// storage valid for the duration of this call.
pub unsafe fn copy_string_view(view: sys::mln_buffer_view) -> Result<String> {
    // SAFETY: The caller promises the string view storage is valid for this
    // call; copy_string_view_bytes copies before returning.
    let bytes = unsafe { copy_string_view_bytes(view) }?;
    String::from_utf8(bytes).map_err(|error| {
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
    if ptr.is_null() {
        return Ok(String::new());
    }

    // SAFETY: The caller promised ptr is a valid NUL-terminated C string.
    let bytes = unsafe { CStr::from_ptr(ptr) }.to_bytes();
    str::from_utf8(bytes)
        .map(str::to_owned)
        .map_err(|error| Error::invalid_argument(format!("native C string was not UTF-8: {error}")))
}

fn embedded_nul_error() -> Error {
    Error::invalid_argument("C string inputs must not contain embedded NUL characters")
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn c_strings_reject_embedded_nul() {
        let error = c_string("a\0b").unwrap_err();

        assert_eq!(error.kind(), crate::error::ErrorKind::InvalidArgument);
        assert_eq!(
            error.diagnostic(),
            "C string inputs must not contain embedded NUL characters"
        );
    }

    #[test]
    fn string_views_materialize_with_explicit_length() {
        let value = "hello";
        let view = string_view(value).raw();

        assert_eq!(view.size, 5);
        assert!(!view.data.is_null());
        assert_eq!(unsafe { copy_string_view(view) }.unwrap(), value);
    }

    #[test]
    fn invalid_native_string_views_are_rejected() {
        let view = sys::mln_buffer_view {
            data: ptr::null(),
            size: 1,
        };

        let error = unsafe { copy_string_view(view) }.unwrap_err();
        assert_eq!(error.kind(), crate::error::ErrorKind::InvalidArgument);
        assert!(error.diagnostic().contains("must not be null"));
    }
}
