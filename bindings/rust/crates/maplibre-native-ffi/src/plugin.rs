use crate::render;

/// Returns the process-lifetime v1 plugin registration function pointer.
/// Pass it to the plugin's own registration entry point before loading dependent styles.
pub fn plugin_register_function_v1() -> render::NativePointer {
    // SAFETY: the native registration function remains valid for the process lifetime.
    unsafe {
        render::NativePointer::from_address(maplibre_native_ffi_core::plugin_register_function_v1())
    }
}
