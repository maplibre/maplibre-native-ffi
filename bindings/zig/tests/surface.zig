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

const spin_turns = 1_000;
const wait_turns = spin_turns + 30_000;

fn waitOneTurn(turn: usize) !void {
    if (turn < spin_turns) return std.Thread.yield();
    try testing.io.sleep(.fromMilliseconds(1), .awake);
}

fn waitForVoid(
    session: maplibre.RenderSessionHandle,
    future_value: maplibre.Future(void),
    service_driver: bool,
) !void {
    var future = future_value;
    defer future.deinit();
    for (0..wait_turns) |turn| {
        if (try future.poll()) return future.wait(null);
        if (service_driver) _ = try session.serviceDriverWork(64);
        try waitOneTurn(turn);
    }
    return error.OperationTimedOut;
}

fn finishAttachment(
    attachment: maplibre.RenderSessionAttachment,
    service_driver: bool,
) !maplibre.RenderSessionHandle {
    errdefer {
        var session = attachment.session;
        session.destroy() catch {};
    }
    try waitForVoid(attachment.session, attachment.completion, service_driver);
    return attachment.session;
}

fn closeSession(session: *maplibre.RenderSessionHandle, service_driver: bool) !void {
    try waitForVoid(session.*, try session.detach(), service_driver);
    try session.destroy();
}

fn renderFrame(
    session: maplibre.RenderSessionHandle,
    token: u64,
    service_driver: bool,
) !maplibre.FrameResult {
    try session.requestFrame(.{ .if_needed = false, .present = true, .token = token });
    for (0..wait_turns) |turn| {
        if (service_driver) _ = try session.serviceDriverWork(64);
        var batch = session.drainFrameResults() catch |err| switch (err) {
            error.NotReady => {
                try waitOneTurn(turn);
                continue;
            },
            else => return err,
        };
        defer batch.release();
        for (0..try batch.count()) |index| {
            const result = try batch.get(index);
            if (result.token == token) return result;
        }
        try waitOneTurn(turn);
    }
    return error.FrameTimedOut;
}

fn expectRenderedFrame(
    session: maplibre.RenderSessionHandle,
    first_token: u64,
    service_driver: bool,
) !maplibre.FrameResult {
    for (first_token..first_token + 1_000) |token| {
        const result = try renderFrame(session, token, service_driver);
        switch (result.disposition) {
            .rendered => return result,
            .size_pending, .target_not_ready => {},
            else => return error.UnexpectedFrameDisposition,
        }
    }
    return error.FrameDidNotRender;
}

fn createMap(runtime: *maplibre.RuntimeHandle, width: u32, height: u32) !maplibre.MapHandle {
    var future = try maplibre.MapHandle.create(runtime, .{
        .width = width,
        .height = height,
    });
    defer future.deinit();
    return future.wait(null);
}

fn descriptor(layer: metal.WindowLayer, extent: maplibre.RenderTargetExtent) !maplibre.MetalSurfaceDescriptor {
    return .{
        .extent = extent,
        .layer = maplibre.NativePointer.fromPtr(try layer.layerPointer()),
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

    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try createMap(&runtime, 32, 16);
    defer map.close() catch @panic("map close failed");

    var session = try finishAttachment(
        try maplibre.attachMetalSurface(
            &map,
            try descriptor(initial_layer, .{ .width = 32, .height = 16, .scale_factor = 2 }),
            .{ .driver = .core_worker },
        ),
        false,
    );
    defer closeSession(&session, false) catch @panic("render session close failed");

    const capabilities = try session.capabilities();
    try testing.expectEqual(.core_worker, capabilities.driver);
    try testing.expect(capabilities.presentation);
    try testing.expect(!capabilities.frame_acquisition);
    try testing.expect(!capabilities.readback);
    try testing.expect(try initial_layer.hasDevice());
    const initial_drawable_size = try initial_layer.drawableSize();
    try testing.expectEqual(@as(u32, 64), initial_drawable_size.width);
    try testing.expectEqual(@as(u32, 32), initial_drawable_size.height);

    try testing.expectEqual(
        .committed,
        try support.waitForCommandDisposition(&runtime, try map.setStyleJson(testing.allocator, support.style_json)),
    );
    try support.waitForBarrier(&runtime);
    const initial_frame = try expectRenderedFrame(session, 1_000, false);
    try testing.expect(initial_frame.frame_generation != 0);
    const initial_drawable_count = try initial_layer.nextDrawableCount();
    try testing.expect(initial_drawable_count != 0);

    try waitForVoid(
        session,
        try session.setMetalSurfaceTarget(try descriptor(replacement_layer, .{ .width = 48, .height = 24 })),
        false,
    );
    try testing.expectEqual(
        .committed,
        try support.waitForCommandDisposition(&runtime, try map.resize(48, 24, 1)),
    );
    try support.waitForBarrier(&runtime);
    _ = try expectRenderedFrame(session, 2_000, false);

    try testing.expectEqual(initial_drawable_count, try initial_layer.nextDrawableCount());
    try testing.expect((try replacement_layer.nextDrawableCount()) != 0);
    const snapshot = try session.snapshot();
    try testing.expectEqual(.attached, std.meta.activeTag(snapshot.state));
    try testing.expectEqual(@as(u32, 48), snapshot.extent.width);
    try testing.expectEqual(@as(u32, 24), snapshot.extent.height);
}

test "Metal surface caller driver presents when the host services it" {
    if (!supports_metal_surface) return error.SkipZigTest;
    const pool = try metal.AutoreleasePool.init();
    defer pool.deinit();

    var layer = try metal.createCountingWindowLayer(24, 12);
    defer layer.deinit();
    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, null);
    defer support.closeRuntime(&runtime) catch @panic("runtime close failed");
    var map = try createMap(&runtime, 24, 12);
    defer map.close() catch @panic("map close failed");

    var session = try finishAttachment(
        try maplibre.attachMetalSurface(
            &map,
            try descriptor(layer, .{ .width = 24, .height = 12 }),
            .{ .driver = .caller_graphics_thread },
        ),
        true,
    );
    defer closeSession(&session, true) catch @panic("render session close failed");

    try testing.expectEqual(.caller_graphics_thread, (try session.capabilities()).driver);
    try testing.expectEqual(
        .committed,
        try support.waitForCommandDisposition(&runtime, try map.setStyleJson(testing.allocator, support.style_json)),
    );
    try support.waitForBarrier(&runtime);
    _ = try expectRenderedFrame(session, 3_000, true);
    try testing.expect((try layer.nextDrawableCount()) != 0);
}
