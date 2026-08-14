const std = @import("std");
const testing = std.testing;
const maplibre = @import("maplibre_native_ffi");

test "surface descriptors stay independent from driver selection" {
    const fake = maplibre.NativePointer.fromPtr(@ptrFromInt(1));
    const descriptor = maplibre.MetalSurfaceDescriptor{
        .extent = .{ .width = 64, .height = 32, .scale_factor = 2 },
        .layer = fake,
    };
    const options = maplibre.RenderSessionAttachOptions{ .driver = .caller_graphics_thread };
    try testing.expectEqual(@as(u32, 64), descriptor.extent.width);
    try testing.expectEqual(maplibre.RenderDriver.caller_graphics_thread, options.driver);
}

test "OpenGL surface selection is explicitly caller-driven" {
    const options = maplibre.RenderSessionAttachOptions{ .driver = .caller_graphics_thread };
    try testing.expectEqual(.caller_graphics_thread, options.driver);
}
