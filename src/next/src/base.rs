// Ports maplibre-native-ffi src/c_api/version.cpp and the base.h /
// render_target.h helpers.

use crate::diagnostics::{Failure, boundary};
use crate::types::*;

#[unsafe(no_mangle)]
pub extern "C" fn mln_c_version() -> u32 {
    MLN_C_VERSION
}

/// The port renders through wgpu, i.e. the WebGPU API.
#[unsafe(no_mangle)]
pub extern "C" fn mln_supported_render_backend_mask() -> u32 {
    MLN_RENDER_BACKEND_FLAG_WEB_GPU
}

/// # Safety
/// C API contract (render_target.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_logical_extent_physical_size(
    extent: mln_logical_extent,
    out_width: *mut u32,
    out_height: *mut u32,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if out_width.is_null() || out_height.is_null() {
                return Err(Failure::invalid_argument("out_width and out_height must not be null"));
            }
            if !(extent.scale_factor.is_finite() && extent.scale_factor > 0.0) {
                return Err(Failure::invalid_argument("scale_factor must be finite and positive"));
            }
            // HeadlessFrontend's physical size: truncation of size * ratio.
            let ratio = extent.scale_factor as f32;
            *out_width = (extent.width as f32 * ratio) as u32;
            *out_height = (extent.height as f32 * ratio) as u32;
            Ok(())
        })
    }
}
