const std = @import("std");
const build_options = @import("build_options");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const metal_support = @import("metal_support.zig");
const support = @import("support.zig");

fn nativePointer(ptr: *anyopaque) maplibre.NativePointer {
    return maplibre.NativePointer.fromPtr(ptr);
}

test "Metal surface renders to window-attached layer through public binding" {
    if (!build_options.supports_metal) return error.SkipZigTest;

    const pool = try metal_support.AutoreleasePool.init();
    defer pool.deinit();

    var window_layer = try metal_support.createWindowLayer(64, 64);
    defer window_layer.deinit();

    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{ .width = 64, .height = 64 });
    defer map.close() catch @panic("map close failed");

    var surface = try maplibre.attachMetalSurface(&map, .{
        .extent = .{ .width = 64, .height = 64 },
        .layer = nativePointer(window_layer.layer.?),
    });
    defer surface.close() catch {};

    _ = try map.setStyleJson(testing.allocator, support.style_json);
    try testing.expect(try support.waitForEvent(&runtime, .map_render_update_available));
    try testing.expectEqual(@as(maplibre.RenderResult, .rendered), try surface.renderUpdate());
}

test "Metal surface render acquires one drawable per frame through public binding" {
    if (!build_options.supports_metal) return error.SkipZigTest;

    const pool = try metal_support.AutoreleasePool.init();
    defer pool.deinit();

    var window_layer = try metal_support.createCountingWindowLayer(64, 64);
    defer window_layer.deinit();

    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{ .width = 64, .height = 64 });
    defer map.close() catch @panic("map close failed");

    var surface = try maplibre.attachMetalSurface(&map, .{
        .extent = .{ .width = 64, .height = 64 },
        .layer = nativePointer(window_layer.layer.?),
    });
    defer surface.close() catch {};

    _ = try map.setStyleJson(testing.allocator, support.style_json);
    try testing.expect(try support.waitForEvent(&runtime, .map_render_update_available));
    try testing.expectEqual(@as(u32, 0), metal_support.nextDrawableCount(window_layer.layer.?));
    try testing.expectEqual(@as(maplibre.RenderResult, .rendered), try surface.renderUpdate());
    try testing.expectEqual(@as(u32, 1), metal_support.nextDrawableCount(window_layer.layer.?));
}

test "Metal surface set target presents through a replacement layer" {
    if (!build_options.supports_metal) return error.SkipZigTest;

    const pool = try metal_support.AutoreleasePool.init();
    defer pool.deinit();

    var window_layer = try metal_support.createCountingWindowLayer(64, 64);
    defer window_layer.deinit();
    var replacement_layer = try metal_support.createCountingWindowLayer(48, 32);
    defer replacement_layer.deinit();

    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{ .width = 64, .height = 64 });
    defer map.close() catch @panic("map close failed");

    var surface = try maplibre.attachMetalSurface(&map, .{
        .extent = .{ .width = 64, .height = 64 },
        .layer = nativePointer(window_layer.layer.?),
    });
    defer surface.close() catch {};

    _ = try map.setStyleJson(testing.allocator, support.style_json);
    try testing.expect(try support.waitForEvent(&runtime, .map_render_update_available));
    try testing.expectEqual(@as(maplibre.RenderResult, .rendered), try surface.renderUpdate());
    try testing.expectEqual(@as(u32, 1), metal_support.nextDrawableCount(window_layer.layer.?));

    // The session presents through whatever replacement surface it is handed.
    try surface.setMetalSurfaceTarget(.{
        .extent = .{ .width = 48, .height = 32 },
        .layer = nativePointer(replacement_layer.layer.?),
    });

    // A texture descriptor names a target this session does not have; the
    // rejection leaves it presenting through the new layer.
    try testing.expectError(error.Unsupported, surface.setMetalBorrowedTextureTarget(.{
        .extent = .{ .width = 48, .height = 32 },
        .physical_width = 48,
        .physical_height = 32,
        .texture = nativePointer(@ptrFromInt(1)),
    }));

    // Target replacement changes only the graphics resource. The map remains
    // the authority for logical extent and scale.
    try testing.expectEqual(@as(maplibre.RenderResult, .size_pending), try surface.renderUpdate());
    _ = try map.resize(48, 32, 1.0);
    try support.waitForBarrier(&runtime);
    const resized = try map.getSize();
    try testing.expectEqual(@as(u32, 48), resized.width);
    try testing.expectEqual(@as(u32, 32), resized.height);
    try testing.expectEqual(@as(maplibre.RenderResult, .rendered), try surface.renderUpdate());
    try testing.expectEqual(@as(u32, 1), metal_support.nextDrawableCount(replacement_layer.layer.?));
    try testing.expectEqual(@as(u32, 1), metal_support.nextDrawableCount(window_layer.layer.?));
}

test "surface public descriptors report invalid native arguments" {
    if (!build_options.supports_metal and !build_options.supports_vulkan) return error.SkipZigTest;
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    if (build_options.supports_metal) {
        const pool = try metal_support.AutoreleasePool.init();
        defer pool.deinit();
        const layer = try metal_support.createLayer();
        try testing.expectError(error.InvalidArgument, maplibre.attachMetalSurface(&map, .{ .extent = .{ .width = 0 }, .layer = nativePointer(layer) }));
        try testing.expectError(error.InvalidArgument, maplibre.attachMetalSurface(&map, .{ .extent = .{ .height = 0 }, .layer = nativePointer(layer) }));
        try testing.expectError(error.InvalidArgument, maplibre.attachMetalSurface(&map, .{ .extent = .{ .scale_factor = 0 }, .layer = nativePointer(layer) }));
    } else if (build_options.supports_vulkan) {
        const fake = nativePointer(@ptrFromInt(1));
        const descriptor = maplibre.VulkanSurfaceDescriptor{
            .context = .{
                .instance = fake,
                .physical_device = fake,
                .device = fake,
                .graphics_queue = fake,
                .graphics_queue_family_index = 0,
                .get_instance_proc_addr = null,
                .get_device_proc_addr = null,
            },
            .surface = fake,
        };
        try testing.expectError(error.InvalidArgument, maplibre.attachVulkanSurface(&map, .{ .extent = .{ .width = 0 }, .context = descriptor.context, .surface = descriptor.surface }));
        try testing.expectError(error.InvalidArgument, maplibre.attachVulkanSurface(&map, .{ .extent = .{ .height = 0 }, .context = descriptor.context, .surface = descriptor.surface }));
        try testing.expectError(error.InvalidArgument, maplibre.attachVulkanSurface(&map, .{ .extent = .{ .scale_factor = 0 }, .context = descriptor.context, .surface = descriptor.surface }));
    }
}

test "unsupported public surface backends report unsupported" {
    if (!build_options.supports_metal and !build_options.supports_vulkan) return error.SkipZigTest;
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    if (build_options.supports_metal) {
        const fake = nativePointer(@ptrFromInt(1));
        try testing.expectError(error.Unsupported, maplibre.attachVulkanSurface(&map, .{
            .context = .{
                .instance = fake,
                .physical_device = fake,
                .device = fake,
                .graphics_queue = fake,
                .graphics_queue_family_index = 0,
                .get_instance_proc_addr = null,
                .get_device_proc_addr = null,
            },
            .surface = fake,
        }));
    } else if (build_options.supports_vulkan) {
        try testing.expectError(error.Unsupported, maplibre.attachMetalSurface(&map, .{ .layer = nativePointer(@ptrFromInt(1)) }));
    }
}
