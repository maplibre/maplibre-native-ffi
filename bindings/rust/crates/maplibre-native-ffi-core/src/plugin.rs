use maplibre_native_ffi_sys as sys;

use crate::{Result, check, string};

/// Loads a layer plugin shared library and registers its style layer types
/// process-wide.
///
/// `path` is the UTF-8 path of the plugin library, and `entry_point` names
/// the exported function that registers the plugin's layer types; the loader
/// calls it with the process-wide register function. The library stays loaded
/// for the process lifetime, because registration retains the plugin's
/// callbacks.
///
/// This function is callable from any thread. Call it before any style that
/// uses the plugin's layer types loads. Loading an identical plugin again
/// succeeds.
///
/// Returns an invalid-argument error when `path` or `entry_point` is empty,
/// and a native error carrying the OS or plugin diagnostic when the library
/// fails to load, the entry point fails to resolve, or registration fails.
pub fn load_plugin(path: &str, entry_point: &str) -> Result<()> {
    let path = string::string_view(path);
    let entry_point = string::string_view(entry_point);
    // SAFETY: path and entry_point are borrowed string views whose storage
    // lives for this call, and the C API validates them.
    check(unsafe { sys::mln_plugin_load_library(path.raw(), entry_point.raw()) })
}
