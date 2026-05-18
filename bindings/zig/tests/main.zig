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
    _ = @import("resources.zig");
    _ = @import("logging.zig");
    _ = @import("render.zig");
    _ = @import("surface.zig");
}

test "package root hides raw C declarations" {
    try testing.expect(!@hasDecl(maplibre, "c"));
    try testing.expect(!@hasDecl(maplibre, "mln_runtime"));
    try testing.expect(!@hasDecl(maplibre, "mln_runtime_create"));
    try testing.expect(!@hasDecl(maplibre.OwnedJsonValue, "copyFromNative"));
    try testing.expect(!support.typeNameContains(maplibre.RuntimeHandle, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.MapHandle, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.MapProjectionHandle, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.CanonicalTileId, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.CustomGeometrySourceOptions, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.CameraOptions, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.AnimationOptions, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.ProjectionMode, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.ViewportOptions, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.TileOptions, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.StyleTileScheme, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.StyleVectorTileEncoding, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.StyleRasterDemEncoding, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.StyleTileSourceOptions, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.PremultipliedRgba8Image, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.StyleImageOptions, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.StyleImageInfo, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.OwnedStyleImage, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.LocationIndicatorImageKind, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.JsonValue, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.OwnedJsonValue, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.Geometry, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.GeoJson, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.StyleSourceInfo, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.NetworkStatus, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.AmbientCacheOperation, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.OfflineRegionDefinition, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.OwnedOfflineRegion, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.OfflineRegionList, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.LogRecord, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.ResourceRequest, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.ResourceResponse, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.ResourceRequestHandle, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.RenderTargetExtent, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.RenderBackendSupport, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.MetalOwnedTextureDescriptor, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.VulkanOwnedTextureDescriptor, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.FeatureStateSelector, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.RenderedQueryGeometry, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.FeatureQueryResult, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.FeatureExtensionResult, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.RenderSessionHandle, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.MetalOwnedTextureFrameHandle, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.MetalOwnedTextureFrameInfo, "mln_"));
    try testing.expect(!support.typeNameContains(maplibre.RuntimeHandle, "anyopaque"));
    try testing.expect(!support.typeNameContains(maplibre.MapHandle, "anyopaque"));
    try testing.expect(!support.typeNameContains(maplibre.MapProjectionHandle, "anyopaque"));
    try testing.expect(!support.typeNameContains(maplibre.RenderSessionHandle, "anyopaque"));
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
