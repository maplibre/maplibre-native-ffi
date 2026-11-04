// Ports maplibre-native-ffi's command groups (src/c_api/map.cpp and
// src/map/map.cpp map_begin_command_group, map_end_command_group): a group
// holds the map's render updates until its outermost end. The map's
// `ProxyFrontend` (session.rs) holds them.

use crate::delivery::Value;
use crate::diagnostics::{Failure, boundary};
use crate::map::command;
use crate::types::*;

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_begin_command_group(
    map: mln_map,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            command(map, completion, true, |entry| {
                entry.frontend.begin_command_group();
                Ok(Value::None)
            })
        })
    }
}

/// # Safety
/// C API contract (map.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_end_command_group(
    map: mln_map,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            command(map, completion, true, |entry| {
                if !entry.frontend.end_command_group() {
                    return Err(Failure::invalid_state("map has no open command group"));
                }
                // The command's own transaction publishes the held update
                // when this was the outermost group.
                Ok(Value::None)
            })
        })
    }
}
