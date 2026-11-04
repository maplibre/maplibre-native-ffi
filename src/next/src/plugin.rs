// The plugin registration entry point that the C++ library exports from
// MapLibre Native (mln/plugin/plugin_api.h), and plugin.h's accessor for it.
// maplibre-native-next has no plugin host, so registration fails.

use crate::types::*;
use std::ffi::{c_char, c_void};

// mln_plugin_status in plugin_api.h.
const MLN_PLUGIN_STATUS_INVALID_ARGUMENT: u32 = 2;
const MLN_PLUGIN_STATUS_UNSUPPORTED_ABI: u32 = 3;

/// # Safety
/// Plugin API contract (mln/plugin/plugin_api.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_plugin_register_v1(
    descriptor: *const c_void,
    error_message: *mut c_char,
    error_message_capacity: usize,
) -> u32 {
    let (status, message) = if descriptor.is_null() {
        (MLN_PLUGIN_STATUS_INVALID_ARGUMENT, "descriptor must not be null")
    } else {
        (MLN_PLUGIN_STATUS_UNSUPPORTED_ABI, "maplibre-native-next does not support plugins yet")
    };
    if !error_message.is_null() && error_message_capacity > 0 {
        // The message, truncated to leave room for its NUL.
        let length = message.len().min(error_message_capacity - 1);
        // SAFETY: the caller's buffer holds `error_message_capacity` bytes.
        unsafe {
            std::ptr::copy_nonoverlapping(message.as_ptr().cast(), error_message, length);
            *error_message.add(length) = 0;
        }
    }
    status
}

/// # Safety
/// C API contract (plugin.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_plugin_get_register_function_v1() -> mln_plugin_register_function_v1 {
    Some(mln_plugin_register_v1)
}
