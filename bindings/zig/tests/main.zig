const testing = @import("std").testing;

const maplibre = @import("maplibre_native");

comptime {
    _ = @import("diagnostics.zig");
    _ = @import("runtime.zig");
    _ = @import("map_lifecycle.zig");
    _ = @import("camera.zig");
    _ = @import("projection.zig");
    _ = @import("map_tuning.zig");
    _ = @import("style_values.zig");
    _ = @import("geojson.zig");
    _ = @import("style_sources.zig");
    _ = @import("resources.zig");
    _ = @import("logging.zig");
    _ = @import("render.zig");
    _ = @import("surface.zig");
}

test "native pointer uses explicit borrowed constructor" {
    const ptr: *anyopaque = @ptrFromInt(1);
    const native = maplibre.NativePointer.fromPtr(ptr);
    try testing.expectEqual(ptr, native.toPtr());
}

test "package links the native C library" {
    try testing.expectEqual(@as(u32, 0), maplibre.cAbiVersion());
}

test "package validates the supported C ABI version" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    try maplibre.validateAbiVersion(&diagnostics);
    try testing.expect(diagnostics.get() == null);
}
