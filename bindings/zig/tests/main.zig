const testing = @import("std").testing;

const maplibre = @import("maplibre_native");
const support = @import("support.zig");

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
}

test "package root hides raw C declarations" {
    try testing.expect(!@hasDecl(maplibre, "c"));
    try testing.expect(!@hasDecl(maplibre, "mln_runtime"));
    try testing.expect(!@hasDecl(maplibre, "mln_runtime_create"));
    try testing.expect(!@hasDecl(maplibre.OwnedJsonValue, "copyFromNative"));
    try testing.expect(!support.typeNameContains(maplibre.RuntimeHandle, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.MapHandle, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.MapProjectionHandle, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.CameraOptions, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.AnimationOptions, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.ProjectionMode, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.ViewportOptions, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.TileOptions, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.JsonValue, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.OwnedJsonValue, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.Geometry, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.GeoJson, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.StyleSourceInfo, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.RuntimeHandle, "anyopaque"));
    try testing.expect(!support.typeNameContains(maplibre.MapHandle, "anyopaque"));
    try testing.expect(!support.typeNameContains(maplibre.MapProjectionHandle, "anyopaque"));
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
