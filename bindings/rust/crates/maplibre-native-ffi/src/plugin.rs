use maplibre_native_ffi_core as maplibre_core;

use crate::Result;

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
/// Returns an error with [`ErrorKind::InvalidArgument`](crate::ErrorKind::InvalidArgument)
/// when `path` or `entry_point` is empty, and with
/// [`ErrorKind::NativeError`](crate::ErrorKind::NativeError) carrying the OS or
/// plugin diagnostic when the library fails to load, the entry point fails to
/// resolve, or registration fails.
///
/// # Safety
///
/// The library must be compatible with this host's plugin ABI. Its entry point
/// must have the declared registration signature, and its initialization and
/// callbacks must uphold Rust's memory and thread safety requirements.
pub unsafe fn load_plugin(path: &str, entry_point: &str) -> Result<()> {
    // SAFETY: the caller supplies the core loader's safety guarantees.
    unsafe { maplibre_core::plugin::load_plugin(path, entry_point) }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ErrorKind;

    #[test]
    fn loading_a_missing_plugin_library_reports_a_native_error() {
        // SAFETY: the missing path cannot load or execute plugin code.
        let error =
            unsafe { load_plugin("/nonexistent/maplibre-plugin.dylib", "mln_plugin_entry") }
                .unwrap_err();

        assert_eq!(error.kind(), ErrorKind::NativeError);
        assert!(!error.diagnostic().is_empty());
    }
}
