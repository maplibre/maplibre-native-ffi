use maplibre_native_ffi_sys as sys;

/// Returns the process-lifetime v1 plugin registration function.
/// Pass it to the plugin's own registration entry point before loading dependent styles.
pub fn plugin_register_function_v1() -> usize {
    // SAFETY: the accessor borrows no data and returns a process-lifetime function.
    unsafe { sys::mln_plugin_get_register_function_v1() }.expect("registration function") as usize
}
