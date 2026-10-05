pub const raw = @import("maplibre_native_c");

// Layer plugin registration stays in this raw layer; see
// include/maplibre_native_c/plugin.h. A null descriptor is rejected before it
// is read, so the call proves the header translates and the entry point links.
test "plugin registration links through the raw layer" {
    const register: raw.mln_plugin_register_function_v1 = @ptrFromInt(@intFromEnum(@import("plugin.zig").pluginRegisterFunctionV1()));
    const status = register.?(null, null, 0);
    try @import("std").testing.expectEqual(@as(raw.mln_plugin_status, raw.MLN_PLUGIN_STATUS_INVALID_ARGUMENT), status);
}
