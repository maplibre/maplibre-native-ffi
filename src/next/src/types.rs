// The C types of maplibre-native-ffi's headers (include/), and the helpers
// this crate builds on them. The declarations are the ones tools/bindgen
// generates for the Rust -sys crate, whose layouts
// tests/bindgen/test_rust_emitter.py checks against the C compiler's. C enums
// are plain integers there, so an unknown value from the caller is never
// undefined behaviour.

// generated.rs takes these from the -sys crate's root. Its handle newtypes
// convert to and from the u64 that handles.rs issues.
macro_rules! native_handles {
    ($($handle:ident),* $(,)?) => {$(
        impl From<$handle> for u64 {
            fn from(handle: $handle) -> u64 {
                handle.0
            }
        }

        impl From<u64> for $handle {
            fn from(raw: u64) -> $handle {
                $handle(raw)
            }
        }
    )*};
}

/// The -sys crate declares this by hand, after upstream's plugin_api.h.
#[allow(non_camel_case_types)]
pub type mln_plugin_register_function_v1 = Option<unsafe extern "C" fn(*const std::ffi::c_void, *mut std::ffi::c_char, usize) -> u32>;

#[allow(dead_code, non_camel_case_types)]
#[path = "../../../bindings/rust/crates/maplibre-native-ffi-sys/src/generated.rs"]
mod generated;

pub use generated::*;

// The headers' #defines, which bindgen doesn't translate.
pub const MLN_DIAGNOSTIC_MESSAGE_CAPACITY: usize = 4096;
pub const MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE: u64 = u64::MAX;

/// Every `MLN_CAMERA_OPTION_*` field.
pub const MLN_CAMERA_OPTION_ALL: u32 = (1 << 9) - 1;

/// `Default` as all-zero fields, for the plain-data structs this crate fills.
macro_rules! zeroed_default {
    ($($ty:ty),* $(,)?) => {$(
        impl Default for $ty {
            fn default() -> Self {
                // SAFETY: plain C data, for which all zeros is a valid value.
                unsafe { std::mem::zeroed() }
            }
        }
    )*};
}
zeroed_default!(
    mln_camera_query_result,
    mln_map_snapshot,
    mln_map_view,
    mln_render_abandon_result,
    mln_render_frame_result,
    mln_rendering_stats,
    mln_runtime_event_payload,
    mln_texture_image_info,
);

/// Text that the library lends to a callback, NUL-terminated.
pub struct LentText(Vec<u8>);

impl LentText {
    pub fn new(text: impl Into<Vec<u8>>) -> LentText {
        let mut bytes = text.into();
        bytes.push(0);
        LentText(bytes)
    }
}

impl mln_buffer_view {
    pub fn of(bytes: &[u8]) -> mln_buffer_view {
        mln_buffer_view { data: bytes.as_ptr().cast(), size: bytes.len() }
    }

    /// A view of lent text: its bytes, followed by a NUL that `size` leaves
    /// out. The header promises no terminator, but the C++ library lends
    /// every text with one and the ffi's tests check it.
    pub fn of_text(text: &LentText) -> mln_buffer_view {
        mln_buffer_view { data: text.0.as_ptr().cast(), size: text.0.len() - 1 }
    }

    /// The bytes, or `None` for a null `data` with a nonzero `size`.
    ///
    /// # Safety
    /// `data` points at `size` readable bytes, or `size` is 0.
    pub unsafe fn bytes<'a>(self) -> Option<&'a [u8]> {
        if self.size == 0 {
            Some(&[])
        } else if self.data.is_null() {
            None
        } else {
            // SAFETY: the caller's contract.
            Some(unsafe { std::slice::from_raw_parts(self.data.cast::<u8>(), self.size) })
        }
    }
}

/// Reads a caller's options struct whose first member is its `uint32_t size`.
///
/// Returns `None` for a null pointer or a `size` smaller than the struct
/// these headers declare; a larger `size` (a newer caller) reads the known
/// prefix.
///
/// # Safety
/// `ptr` is null or points at a readable `T` whose first field is `u32 size`.
pub unsafe fn read_sized<T: Copy>(ptr: *const T) -> Option<T> {
    if ptr.is_null() {
        return None;
    }
    // SAFETY: the caller's contract; `size` is the first field.
    let size = unsafe { *ptr.cast::<u32>() } as usize;
    if size < std::mem::size_of::<T>() {
        return None;
    }
    // SAFETY: as above.
    Some(unsafe { *ptr })
}

/// Writes `value` to a caller's out struct whose `size` field says how large
/// it is; fails like `read_sized` for a null pointer or a short struct.
///
/// # Safety
/// `ptr` is null or points at a writable `T` whose first field is `u32 size`.
pub unsafe fn write_sized<T: Copy>(ptr: *mut T, mut value: T) -> bool {
    if ptr.is_null() {
        return false;
    }
    // SAFETY: the caller's contract.
    let size = unsafe { *ptr.cast::<u32>() };
    if (size as usize) < std::mem::size_of::<T>() {
        return false;
    }
    // Keep the caller's size, as C++ does.
    // SAFETY: `value` is a `T` whose first field is the `u32 size`.
    unsafe { *(&mut value as *mut T).cast::<u32>() = size };
    // SAFETY: the caller's contract.
    unsafe { *ptr = value };
    true
}
