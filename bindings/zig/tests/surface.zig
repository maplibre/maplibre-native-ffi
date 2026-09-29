const builtin = @import("builtin");
const std = @import("std");
const build_options = @import("build_options");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("support.zig");

const supports_metal_surface = build_options.supports_metal and switch (builtin.os.tag) {
    .macos, .ios, .tvos => true,
    else => false,
};
const metal = if (supports_metal_surface) @import("metal_support.zig") else struct {};

fn metalSurfaceDescriptor(layer: metal.WindowLayer, extent: maplibre.RenderTargetExtent) !maplibre.MetalSurfaceDescriptor {
    return .{
        .extent = extent,
        .layer = try layer.layerPointer(),
    };
}

test "Metal surface core worker presents and replaces its target" {
    if (!supports_metal_surface) return error.SkipZigTest;
    const pool = try metal.AutoreleasePool.init();
    defer pool.deinit();

    var initial_layer = try metal.createCountingWindowLayer(32, 16);
    defer initial_layer.deinit();
    var replacement_layer = try metal.createCountingWindowLayer(48, 24);
    defer replacement_layer.deinit();

    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 32, .height = 16, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");

    var session = try support.finishAttachment(
        try maplibre.metalSurfaceAttach(
            testing.allocator,
            support.handle(map),
            try metalSurfaceDescriptor(initial_layer, .{ .width = 32, .height = 16, .scale_factor = 2 }),
            .{ .driver = .core_worker },
            null,
        ),
        false,
    );
    defer support.closeSession(&session, false) catch @panic("render session close failed");

    const capabilities = try maplibre.renderSessionGetCapabilities(support.handle(session), null);
    try testing.expectEqual(.core_worker, capabilities.driver);
    try testing.expect(capabilities.flags.presentation);
    try testing.expect(!capabilities.flags.frame_acquisition);
    try testing.expect(!capabilities.flags.readback);
    try testing.expect(try initial_layer.hasDevice());
    const initial_drawable_size = try initial_layer.drawableSize();
    try testing.expectEqual(@as(u32, 64), initial_drawable_size.width);
    try testing.expectEqual(@as(u32, 32), initial_drawable_size.height);

    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), support.style_json, null));
    try support.waitForBarrier(&runtime);
    const initial_frame = try support.expectRenderedFrame(session, false);
    try testing.expect(initial_frame.frame_generation != 0);
    const initial_drawable_count = try initial_layer.nextDrawableCount();
    try testing.expect(initial_drawable_count != 0);

    try support.finishOperation(
        session,
        try maplibre.metalSurfaceSetTarget(testing.allocator, session, try metalSurfaceDescriptor(replacement_layer, .{ .width = 48, .height = 24, .scale_factor = 1.0 }), null),
        false,
    );
    // The session resize is the single authority for an attached session: it
    // posts the map resize itself.
    try support.finishOperation(session, try maplibre.renderSessionResize(testing.allocator, support.handle(session), .{ .width = 48, .height = 24, .scale_factor = 1.0 }, null), false);
    try support.waitForBarrier(&runtime);
    _ = try support.expectRenderedFrame(session, false);

    try testing.expectEqual(initial_drawable_count, try initial_layer.nextDrawableCount());
    try testing.expect((try replacement_layer.nextDrawableCount()) != 0);
    const snapshot = try maplibre.renderSessionGetSnapshot(session, null);
    try testing.expectEqual(.attached, snapshot.state);
    try testing.expectEqual(@as(u32, 48), snapshot.extent.width);
    try testing.expectEqual(@as(u32, 24), snapshot.extent.height);
}

test "Metal surface caller driver presents when the host services it" {
    if (!supports_metal_surface) return error.SkipZigTest;
    const pool = try metal.AutoreleasePool.init();
    defer pool.deinit();

    var layer = try metal.createCountingWindowLayer(24, 12);
    defer layer.deinit();
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 24, .height = 12, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");

    var session = try support.finishAttachment(
        try maplibre.metalSurfaceAttach(
            testing.allocator,
            support.handle(map),
            try metalSurfaceDescriptor(layer, .{ .width = 24, .height = 12, .scale_factor = 1.0 }),
            .{ .driver = .caller_graphics_thread },
            null,
        ),
        true,
    );
    defer support.closeSession(&session, true) catch @panic("render session close failed");

    try testing.expectEqual(.caller_graphics_thread, (try maplibre.renderSessionGetCapabilities(support.handle(session), null)).driver);
    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), support.style_json, null));
    try support.waitForBarrier(&runtime);
    _ = try support.expectRenderedFrame(session, true);
    try testing.expect((try layer.nextDrawableCount()) != 0);
}

// A demand that clears the present bit still renders, and the target keeps
// whatever it presented last.
test "Metal surface honors the present bit on each demand" {
    if (!supports_metal_surface) return error.SkipZigTest;
    const pool = try metal.AutoreleasePool.init();
    defer pool.deinit();

    var layer = try metal.createCountingWindowLayer(32, 16);
    defer layer.deinit();
    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 32, .height = 16, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");

    var session = try support.finishAttachment(
        try maplibre.metalSurfaceAttach(
            testing.allocator,
            support.handle(map),
            try metalSurfaceDescriptor(layer, .{ .width = 32, .height = 16, .scale_factor = 1.0 }),
            .{ .driver = .core_worker },
            null,
        ),
        false,
    );
    defer support.closeSession(&session, false) catch @panic("render session close failed");

    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), support.style_json, null));
    try support.waitForBarrier(&runtime);
    const presenting_start = try support.expectRenderedFrame(session, false);
    const presented = try layer.nextDrawableCount();
    try testing.expect(presented != 0);

    const nonpresenting = try support.renderFrameWithDemand(session, .{
        .flags = .{ .if_needed = false, .present = false },
        .token = support.nextFrameToken(),
    }, false);
    try testing.expectEqual(.rendered, nonpresenting.disposition);
    try testing.expect(nonpresenting.frame_generation > presenting_start.frame_generation);

    const presenting = try support.renderFrameWithDemand(session, .{
        .flags = .{ .if_needed = false, .present = true },
        .token = support.nextFrameToken(),
    }, false);
    try testing.expectEqual(.rendered, presenting.disposition);
    try testing.expect((try layer.nextDrawableCount()) > presented);
}

test "Metal borrowed texture renders and replaces its target" {
    if (!supports_metal_surface) return error.SkipZigTest;
    const pool = try metal.AutoreleasePool.init();
    defer pool.deinit();

    var initial = try metal.createBorrowedTexture(32, 16);
    defer initial.deinit();
    var replacement = try metal.createBorrowedTexture(48, 24);
    defer replacement.deinit();

    var runtime = try support.createRuntime(.{});
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try support.createMap(&runtime, .{ .width = 32, .height = 16, .scale_factor = 1.0 });
    defer support.closeMap(&map) catch @panic("map close failed");

    var session = try support.finishAttachment(
        try maplibre.metalBorrowedTextureAttach(testing.allocator, support.handle(map), initial.descriptor(.{ .width = 32, .height = 16, .scale_factor = 1.0 }), .{
            .driver = .core_worker,
        }, null),
        false,
    );
    defer support.closeSession(&session, false) catch @panic("render session close failed");

    // A borrowed texture is the host's, so the session grants neither frame
    // acquisition nor readback.
    const capabilities = try maplibre.renderSessionGetCapabilities(support.handle(session), null);
    try testing.expect(!capabilities.flags.frame_acquisition);
    try testing.expect(!capabilities.flags.readback);
    try testing.expect(!capabilities.flags.presentation);

    try support.expectCommitted(try maplibre.mapSetStyleJson(support.handle(map), support.style_json, null));
    try support.waitForBarrier(&runtime);
    _ = try support.expectRenderedFrame(session, false);
    try testing.expect(try initial.hasNonZeroPixel());

    try support.finishOperation(
        session,
        try maplibre.metalBorrowedTextureSetTarget(testing.allocator, session, replacement.descriptor(.{ .width = 48, .height = 24, .scale_factor = 1.0 }), null),
        false,
    );
    // A borrowed texture belongs to the host, so the session cannot resize it;
    // the replacement target carries the new size and the map takes it here.
    try testing.expectError(error.Unsupported, maplibre.renderSessionResize(testing.allocator, support.handle(session), .{ .width = 48, .height = 24, .scale_factor = 1.0 }, null));
    try support.expectCommitted(try maplibre.mapResize(support.handle(map), .{ .width = 48, .height = 24, .scale_factor = 1.0 }, null));
    try support.waitForBarrier(&runtime);
    _ = try support.expectRenderedFrame(session, false);
    try testing.expect(try replacement.hasNonZeroPixel());

    const snapshot = try maplibre.renderSessionGetSnapshot(session, null);
    try testing.expectEqual(@as(u32, 48), snapshot.extent.width);
    try testing.expectEqual(@as(u32, 24), snapshot.extent.height);
}
