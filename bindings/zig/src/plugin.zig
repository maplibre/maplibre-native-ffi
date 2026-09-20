const NativePointer = @import("render.zig").NativePointer;
const c = @import("c.zig").raw;

/// Returns the process-lifetime v1 plugin registration function.
/// Pass it to the plugin's own registration entry point before loading dependent styles.
pub fn pluginRegisterFunctionV1() NativePointer {
    return @enumFromInt(@intFromPtr(c.mln_plugin_get_register_function_v1().?));
}
