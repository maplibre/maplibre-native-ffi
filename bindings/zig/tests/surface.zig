const std = @import("std");
const build_options = @import("build_options");
const testing = std.testing;

const maplibre = @import("maplibre_native");
const metal_support = @import("metal_support.zig");
const support = @import("support.zig");

fn waitForEvent(runtime: *maplibre.RuntimeHandle, event_type: maplibre.RuntimeEventType) !bool {
    for (0..1000) |_| {
        try runtime.runOnce();
        while (try runtime.pollEvent()) |event| {
            if (std.meta.eql(event.event_type, event_type)) return true;
        }
        try std.Thread.yield();
    }
    return false;
}

fn nativePointer(ptr: *anyopaque) maplibre.NativePointer {
    return .{ .ptr = ptr };
}

test "Metal surface renders to window-attached layer through public binding" {
    if (!build_options.supports_metal) return error.SkipZigTest;

    const pool = try metal_support.AutoreleasePool.init();
    defer pool.deinit();

    var window_layer = metal_support.createWindowLayer(64, 64) catch return error.SkipZigTest;
    defer window_layer.deinit();

    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    var surface = try maplibre.attachMetalSurface(&map, .{
        .extent = .{ .width = 64, .height = 64 },
        .layer = nativePointer(window_layer.layer.?),
    });
    defer surface.close() catch {};

    try map.setStyleJson(testing.allocator, support.style_json);
    try testing.expect(try waitForEvent(&runtime, .map_render_update_available));
    try surface.renderUpdate();
}

test "Metal surface render acquires one drawable per frame through public binding" {
    if (!build_options.supports_metal) return error.SkipZigTest;

    const pool = try metal_support.AutoreleasePool.init();
    defer pool.deinit();

    var window_layer = metal_support.createCountingWindowLayer(64, 64) catch return error.SkipZigTest;
    defer window_layer.deinit();

    var runtime = try maplibre.RuntimeHandle.init(null);
    defer runtime.close() catch @panic("runtime close failed");
    var map = try maplibre.MapHandle.create(&runtime, .{});
    defer map.close() catch @panic("map close failed");

    var surface = try maplibre.attachMetalSurface(&map, .{
        .extent = .{ .width = 64, .height = 64 },
        .layer = nativePointer(window_layer.layer.?),
    });
    defer surface.close() catch {};

    try map.setStyleJson(testing.allocator, support.style_json);
    try testing.expect(try waitForEvent(&runtime, .map_render_update_available));
    try testing.expectEqual(@as(u32, 0), metal_support.nextDrawableCount(window_layer.layer.?));
    try surface.renderUpdate();
    try testing.expectEqual(@as(u32, 1), metal_support.nextDrawableCount(window_layer.layer.?));
}

test "surface public descriptors report invalid native arguments" {
    var runtime = try maplibre.RuntimeHandle.init(null);
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
    var runtime = try maplibre.RuntimeHandle.init(null);
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
