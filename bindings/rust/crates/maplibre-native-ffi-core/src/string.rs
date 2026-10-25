use std::str;

use maplibre_native_ffi_sys as sys;

use crate::error::{Error, Result};

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

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn invalid_native_string_views_are_rejected() {
        let view = sys::mln_buffer_view {
            data: std::ptr::null(),
            size: 1,
        };

        let error = unsafe { copy_string_view(view) }.unwrap_err();
        assert_eq!(error.status(), crate::Status::InvalidArgument);
        assert!(error.diagnostic().contains("must not be null"));
    }
}
