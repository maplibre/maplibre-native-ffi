// Ports maplibre-native-ffi src/c_api/version.cpp and the base.h /
// render_target.h helpers.

use crate::diagnostics::{Failure, boundary};
use crate::types::*;

#[unsafe(no_mangle)]
pub extern "C" fn mln_c_version() -> u32 {
    MLN_C_VERSION
}

/// The port renders through wgpu: on Apple platforms to Metal surfaces
/// (metal_surface.rs), elsewhere through the WebGPU API.
#[unsafe(no_mangle)]
pub extern "C" fn mln_supported_render_backend_mask() -> u32 {
    if cfg!(target_vendor = "apple") {
        MLN_RENDER_BACKEND_FLAG_METAL
    } else {
        MLN_RENDER_BACKEND_FLAG_WEB_GPU
    }
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
            if extent.width == 0
                || extent.height == 0
                || !(extent.scale_factor.is_finite() && extent.scale_factor > 0.0)
            {
                return Err(Failure::invalid_argument("extent dimensions and scale_factor must be positive"));
            }
            // C++ `physical_dimension`: the product, rounded up, as a Metal
            // surface session sizes its layer. (An offscreen session's
            // HeadlessFrontend truncates it.)
            let width = (f64::from(extent.width) * extent.scale_factor).ceil();
            let height = (f64::from(extent.height) * extent.scale_factor).ceil();
            if width > f64::from(u32::MAX) || height > f64::from(u32::MAX) {
                return Err(Failure::invalid_argument("scaled extent dimensions are too large"));
            }
            *out_width = width as u32;
            *out_height = height as u32;
            Ok(())
        })
    }
}
