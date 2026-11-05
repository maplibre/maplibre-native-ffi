// The C types of the headers in include/, as tools/bindgen generates them for
// the Rust -sys crate (whose layouts tests/bindgen checks against the C
// compiler's). C enums are plain integers there, so an unknown value from the
// caller is never undefined behaviour.

#![allow(dead_code, non_camel_case_types, non_upper_case_globals, clippy::all)]

// generated.rs implements this macro's trait in the -sys crate; nothing here
// needs it.
macro_rules! native_handles {
    ($($handle:ident),* $(,)?) => {};
}

#[path = "../../../bindings/rust/crates/maplibre-native-ffi-sys/src/generated.rs"]
mod generated;

pub use generated::*;

/// The -sys crate declares the plugin types by hand, after upstream's
/// plugin_api.h.
#[repr(C)]
pub struct mln_plugin_descriptor_v1 {
    _opaque: [u8; 0],
}
pub type mln_plugin_status = std::ffi::c_uint;
pub type mln_plugin_register_function_v1 = Option<
    unsafe extern "C" fn(
        *const mln_plugin_descriptor_v1,
        *mut std::ffi::c_char,
        usize,
    ) -> mln_plugin_status,
>;

/// `Default` as all-zero fields, for plain C data this crate fills.
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
    mln_map_snapshot,
    mln_map_view,
    mln_camera,
    mln_camera_options,
    mln_render_frame_result,
    mln_render_session_snapshot,
    mln_render_session_capabilities,
    mln_runtime_event,
    mln_texture_readback_result,
);
