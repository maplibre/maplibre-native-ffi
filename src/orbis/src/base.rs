// What every entry point shares: failures and diagnostics, the panic boundary,
// buffer views, and the few functions of base.h.

use std::ffi::c_void;
use std::panic::{AssertUnwindSafe, catch_unwind};

use crate::types::*;

/// A failed call: its status and message.
#[derive(Clone, Debug)]
pub struct Failure {
    pub status: mln_status,
    pub message: String,
}

impl Failure {
    pub fn new(status: mln_status, message: impl Into<String>) -> Self {
        Self {
            status,
            message: message.into(),
        }
    }
    pub fn argument(message: impl Into<String>) -> Self {
        Self::new(MLN_STATUS_INVALID_ARGUMENT, message)
    }
    pub fn state(message: impl Into<String>) -> Self {
        Self::new(MLN_STATUS_INVALID_STATE, message)
    }
    pub fn unsupported(message: impl Into<String>) -> Self {
        Self::new(MLN_STATUS_UNSUPPORTED, message)
    }
    pub fn native(message: impl Into<String>) -> Self {
        Self::new(MLN_STATUS_NATIVE_ERROR, message)
    }
}

impl From<orb_api::Error> for Failure {
    fn from(error: orb_api::Error) -> Self {
        let status = match &error {
            orb_api::Error::InvalidArgument(_) => MLN_STATUS_INVALID_ARGUMENT,
            orb_api::Error::InvalidState(_) => MLN_STATUS_INVALID_STATE,
            orb_api::Error::Unsupported(_) => MLN_STATUS_UNSUPPORTED,
            orb_api::Error::TargetLost(_) => MLN_STATUS_TARGET_LOST,
            orb_api::Error::Native(_) | orb_api::Error::DeviceLost => MLN_STATUS_NATIVE_ERROR,
        };
        Self::new(status, error.message())
    }
}

pub type Result<T = ()> = std::result::Result<T, Failure>;

/// Writes `message` into the caller's diagnostic, cut to fit at a character
/// boundary, or nothing when the caller passed none.
///
/// # Safety
/// `diagnostic` is null or a writable `mln_diagnostic` whose `size` says how
/// many of its bytes are writable.
pub unsafe fn write_diagnostic(diagnostic: *mut mln_diagnostic, message: &str) {
    if diagnostic.is_null() {
        return;
    }
    let offset = std::mem::offset_of!(mln_diagnostic, message);
    // SAFETY: the caller's contract.
    let size = unsafe { (*diagnostic).size } as usize;
    if size <= offset {
        return;
    }
    let capacity = (size - offset).min(MLN_DIAGNOSTIC_MESSAGE_CAPACITY as usize);
    let mut length = message.len().min(capacity - 1);
    while !message.is_char_boundary(length) {
        length -= 1;
    }
    // SAFETY: `capacity` bytes of the message are writable.
    unsafe {
        let out = (*diagnostic).message.as_mut_ptr().cast::<u8>();
        std::ptr::copy_nonoverlapping(message.as_ptr(), out, length);
        *out.add(length) = 0;
    }
}

pub fn panic_message(panic: &(dyn std::any::Any + Send)) -> String {
    panic
        .downcast_ref::<&str>()
        .map(|s| (*s).to_owned())
        .or_else(|| panic.downcast_ref::<String>().cloned())
        .unwrap_or_else(|| "a panic in the Orbis implementation".into())
}

/// Runs an entry point's body: its failure becomes the status and the
/// diagnostic, and a panic stops at the C boundary.
///
/// # Safety
/// As [`write_diagnostic`].
pub unsafe fn boundary(
    diagnostic: *mut mln_diagnostic,
    body: impl FnOnce() -> Result,
) -> mln_status {
    let outcome = catch_unwind(AssertUnwindSafe(body))
        .unwrap_or_else(|panic| Err(Failure::native(panic_message(&*panic))));
    match outcome {
        Ok(()) => {
            // SAFETY: the caller's contract.
            unsafe { write_diagnostic(diagnostic, "") };
            MLN_STATUS_OK
        }
        Err(failure) => {
            // SAFETY: the caller's contract.
            unsafe { write_diagnostic(diagnostic, &failure.message) };
            failure.status
        }
    }
}

/// The diagnostic of a function that this implementation lacks.
///
/// # Safety
/// As [`write_diagnostic`].
pub unsafe fn unsupported(diagnostic: *mut mln_diagnostic, name: &str) -> mln_status {
    // SAFETY: the caller's contract.
    unsafe {
        write_diagnostic(diagnostic, &format!("{name} isn't implemented on Orbis"));
    }
    MLN_STATUS_UNSUPPORTED
}

/// The bytes of a buffer view, for the call.
///
/// # Safety
/// `view` names `size` readable bytes, or none with a null pointer.
pub unsafe fn bytes<'a>(view: mln_buffer_view) -> &'a [u8] {
    if view.data.is_null() || view.size == 0 {
        &[]
    } else {
        // SAFETY: the caller's contract.
        unsafe { std::slice::from_raw_parts(view.data.cast(), view.size) }
    }
}

/// A buffer view as UTF-8.
///
/// # Safety
/// As [`bytes`].
pub unsafe fn text(view: mln_buffer_view, what: &str) -> Result<String> {
    // SAFETY: the caller's contract.
    let bytes = unsafe { bytes(view) };
    std::str::from_utf8(bytes)
        .map(str::to_owned)
        .map_err(|_| Failure::argument(format!("{what} isn't UTF-8")))
}

/// A view of bytes the library lends; an empty one still points at a byte.
pub fn view(bytes: &[u8]) -> mln_buffer_view {
    let data = if bytes.is_empty() {
        b"\0".as_ptr()
    } else {
        bytes.as_ptr()
    };
    mln_buffer_view {
        data: data.cast(),
        size: bytes.len(),
    }
}

/// Text as the library lends it: NUL-terminated after `size` bytes. The text
/// gains its terminator, so it must not change while the view is lent.
pub fn lend(text: &mut String) -> mln_buffer_view {
    if !text.ends_with('\0') {
        text.push('\0');
    }
    let bytes = text.as_bytes();
    mln_buffer_view {
        data: bytes.as_ptr().cast(),
        size: bytes.len() - 1,
    }
}

/// A read of a caller's struct whose `size` field comes first: null, or a
/// size smaller than the fields this implementation reads, is an error.
///
/// # Safety
/// `pointer` is null or points at a readable `T` of at least its `size`.
pub unsafe fn sized<'a, T>(pointer: *const T, what: &str) -> Result<&'a T> {
    if pointer.is_null() {
        return Err(Failure::argument(format!("{what} is null")));
    }
    // SAFETY: the caller's contract; every sized struct starts with its u32 size.
    let size = unsafe { *pointer.cast::<u32>() } as usize;
    if size < std::mem::size_of::<T>() {
        return Err(Failure::argument(format!(
            "{what}.size is {size}, smaller than {}",
            std::mem::size_of::<T>()
        )));
    }
    // SAFETY: as above.
    Ok(unsafe { &*pointer })
}

/// Writes `value` through an out pointer.
///
/// # Safety
/// `out` is null or writable.
pub unsafe fn put<T>(out: *mut T, value: T, what: &str) -> Result {
    if out.is_null() {
        return Err(Failure::argument(format!("{what} is null")));
    }
    // SAFETY: the caller's contract.
    unsafe { out.write(value) };
    Ok(())
}

/// A caller's pointer that crosses threads with the callback it goes with.
#[derive(Clone, Copy, Debug)]
pub struct UserData(pub *mut c_void);
// SAFETY: the C API lets the library call a registration's callback, with its
// user_data, on any library thread.
unsafe impl Send for UserData {}
unsafe impl Sync for UserData {}

impl UserData {
    pub fn get(self) -> *mut c_void {
        self.0
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn mln_c_version() -> u32 {
    MLN_C_VERSION
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_buffer_view_from_cstr(
    cstr: *const std::ffi::c_char,
) -> mln_buffer_view {
    if cstr.is_null() {
        return mln_buffer_view {
            data: std::ptr::null(),
            size: 0,
        };
    }
    // SAFETY: a NUL-terminated string, by the header's contract.
    let length = unsafe { std::ffi::CStr::from_ptr(cstr) }.to_bytes().len();
    mln_buffer_view {
        data: cstr.cast(),
        size: length,
    }
}

/// Orbis renders with Metal on Apple platforms and Vulkan elsewhere (wgpu),
/// and through GLES on its Compat tier.
#[unsafe(no_mangle)]
pub extern "C" fn mln_supported_render_backend_mask() -> mln_render_backend_flags {
    if cfg!(target_vendor = "apple") {
        MLN_RENDER_BACKEND_FLAG_METAL
    } else {
        MLN_RENDER_BACKEND_FLAG_VULKAN
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn mln_open_gl_supported_context_provider_mask() -> mln_open_gl_context_provider_flags
{
    0
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_logical_extent_physical_size(
    extent: mln_logical_extent,
    out_width: *mut u32,
    out_height: *mut u32,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: the caller's pointers.
    unsafe {
        boundary(out_diagnostic, || {
            let (width, height) = physical(extent)?;
            put(out_width, width, "out_width")?;
            put(out_height, height, "out_height")
        })
    }
}

/// The pixels of a logical extent, rounded as MapLibre Native rounds them.
pub fn physical(extent: mln_logical_extent) -> Result<(u32, u32)> {
    if extent.width == 0 || extent.height == 0 {
        return Err(Failure::argument("the extent is empty"));
    }
    if !(extent.scale_factor.is_finite() && extent.scale_factor > 0.0) {
        return Err(Failure::argument("the scale factor isn't positive"));
    }
    let px = |v: u32| (f64::from(v) * extent.scale_factor).round() as u32;
    Ok((px(extent.width).max(1), px(extent.height).max(1)))
}
