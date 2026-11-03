//! Render sessions on the build's backend: offscreen, or borrowed textures
//! from tests/graphics.

const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("fixture.zig");

const extent = maplibre.LogicalExtent{ .width = 32, .height = 16, .scale_factor = 1.0 };

fn createFixture() !*support.Fixture {
    const fixture = try support.Fixture.create(.{ .extent = extent });
    errdefer fixture.destroy();
    try fixture.loadStyle();
    return fixture;
}

test "an offscreen session renders a frame that reads back as pixels" {
    const fixture = try createFixture();
    defer fixture.destroy();
    const offscreen = try support.TextureSession.attachOffscreen(fixture.map, extent);
    defer offscreen.close();
    const capabilities = try offscreen.session.getCapabilities(null);
    try testing.expect(capabilities.flags.readback);
    _ = try offscreen.renderUntilRendered();
    var image = try offscreen.resolve(try offscreen.session.readTexture(testing.allocator, null));
    defer image.deinit();
    try testing.expectEqual(extent.width, image.value.info.width);
    try testing.expectEqual(extent.height, image.value.info.height);
    try testing.expectEqual(image.value.info.byte_length, image.value.data.len);
    const offset = (image.value.info.height / 2) * image.value.info.stride + (image.value.info.width / 2) * 4;
    for (support.background_rgba, image.value.data[offset..][0..4]) |expected, actual| {
        try testing.expect(@abs(@as(i16, expected) - @as(i16, actual)) <= 2);
    }
}

// A fetch callback borrows its map, and so does any copy of the value that the
// binding lends it: release and dispose fail, and with deinit they leave the
// caller's map working.
test "a copy of a custom source's borrowed map can't release it" {
    const fixture = try createFixture();
    defer fixture.destroy();
    const Probe = struct {
        claimed: std.atomic.Value(bool) = .init(false),
        fetched: support.Flag = .{},
        dispose_error: ?anyerror = null,
        release_error: ?anyerror = null,
        message: [128]u8 = undefined,
        message_len: usize = 0,

        fn fetchTile(self: *@This(), map: *const maplibre.Map, source_id: []const u8, tile: maplibre.CanonicalTileId) anyerror!void {
            if (!self.claimed.swap(true, .acq_rel)) {
                var copied = map.*;
                var diagnostic: maplibre.Diagnostic = .{};
                if (copied.dispose(&diagnostic)) |_| {} else |err| self.dispose_error = err;
                const message = diagnostic.message();
                self.message_len = @min(message.len, self.message.len);
                @memcpy(self.message[0..self.message_len], message[0..self.message_len]);
                if (copied.release(null)) |released| {
                    var future = released;
                    future.deinit();
                } else |err| self.release_error = err;
                copied.deinit();
            }
            try map.discarding().setCustomGeometrySourceTileData(source_id, tile, "{\"type\":\"FeatureCollection\",\"features\":[]}", null);
            self.fetched.set();
        }
    };
    var probe = Probe{};
    try support.expectCommitted(try fixture.map.addCustomGeometrySource("custom", .init(&probe, Probe.fetchTile, null, null), null));
    try support.expectCommitted(try fixture.map.addStyleLayerJson("{\"id\":\"dots\",\"type\":\"circle\",\"source\":\"custom\"}", null, null));
    const offscreen = try support.TextureSession.attachOffscreen(fixture.map, extent);
    defer offscreen.close();
    // The map asks for its tiles as it renders.
    var frame: usize = 0;
    while (frame < 32 and !probe.fetched.isSet()) : (frame += 1) _ = try offscreen.renderFrame();
    try probe.fetched.wait();
    try testing.expectEqual(@as(?anyerror, error.InvalidState), probe.dispose_error);
    try testing.expectEqualStrings("Map is borrowed by a callback, which can't release it", probe.message[0..probe.message_len]);
    try testing.expectEqual(@as(?anyerror, error.InvalidState), probe.release_error);
    try testing.expect(!fixture.map.isClosed());
    try support.expectCommitted(try fixture.map.requestRepaint(null));
}

// Before any demand, the drain and the acquire write null handles, which read
// as empty and as null.
test "a drain before any demand returns no batch" {
    const fixture = try createFixture();
    defer fixture.destroy();
    const borrowed = try support.TextureSession.attachBorrowed(fixture.map, extent, support.TextureSession.borrowed_driver);
    defer borrowed.close();
    var results = try borrowed.session.drainFrameResults(testing.allocator, null);
    defer results.deinit();
    try testing.expectEqual(@as(usize, 0), results.value.len);
    try testing.expect(try borrowed.session.acquireFrame(null) == null);
}

// A scoped view leases its frame for the callback. Inside the scope, releasing
// that frame or abandoning its session reports busy, and a host error passes
// through. Discarding a sibling frame, which has no consumer fence, leaves the
// view readable. Once the scope returns, the frame releases normally.
test "a scoped frame view holds its frame until the scope returns" {
    const fixture = try createFixture();
    defer fixture.destroy();
    const borrowed = try support.TextureSession.attachBorrowed(fixture.map, extent, support.TextureSession.borrowed_driver);
    defer borrowed.close();
    _ = try borrowed.renderUntilRendered();
    var frame = (try borrowed.session.acquireFrame(null)).?;
    try support.expectCommitted(try fixture.map.requestRepaint(null));
    _ = try borrowed.renderUntilRendered();
    var sibling = (try borrowed.session.acquireFrame(null)).?;

    const width = try support.getFrameTexture(frame, {}, struct {
        fn use(_: void, view: support.FrameTexture) u32 {
            return view.width;
        }
    }.use);
    try testing.expectEqual(extent.width, width);

    const Probe = struct {
        frame: *maplibre.AcquiredFrame,
        sibling: *maplibre.AcquiredFrame,
        session: maplibre.RenderSession,
        fn inspect(self: @This(), _: maplibre.GpuSync) anyerror!void {
            // The native scope keeps the frame and its session busy.
            var diagnostic: maplibre.Diagnostic = .{};
            try testing.expectError(error.Busy, self.frame.release(.{ .kind = .cpu_complete }, &diagnostic));
            try testing.expectEqual(.busy, diagnostic.status);
            try testing.expectError(error.Busy, self.session.abandon(null));
            try self.sibling.discard(null);
            try self.frame.getProducerSync({}, struct {
                fn use(_: void, _: maplibre.GpuSync) anyerror!void {}
            }.use, null);
            return error.HostConsumerFailed;
        }
    };
    try testing.expectError(error.HostConsumerFailed, frame.getProducerSync(Probe{ .frame = &frame, .sibling = &sibling, .session = borrowed.session }, Probe.inspect, null));
    try testing.expect(sibling.isClosed() and !frame.isClosed());
    try frame.release(.{ .kind = .cpu_complete }, null);
    try testing.expect(frame.isClosed());
    // Either consuming call on a closed frame does nothing.
    try frame.release(.{ .kind = .cpu_complete }, null);
    try frame.discard(null);
}

// A frame anchors its session and a session its map. Disposing the map first
// leaves its children to retire with it, and each cleanup is safe in any order.
test "disposing a parent graph retires its acquired frame and session" {
    const fixture = try createFixture();
    defer fixture.destroy();
    const borrowed = try support.TextureSession.attachBorrowed(fixture.map, extent, support.TextureSession.borrowed_driver);
    defer borrowed.close();
    _ = try borrowed.renderUntilRendered();
    var frame = (try borrowed.session.acquireFrame(null)).?;

    fixture.map.deinit();
    try frame.discard(null);
    borrowed.session.deinit();
    // The textures and graphics context outlive the session, so the test waits
    // for the runtime to retire everything before the deferred teardown
    // destroys them, also when a check fails.
    {
        errdefer fixture.releaseRuntimeWhenChildless() catch {};
        try testing.expectError(error.InvalidState, frame.getResult(null));
        try testing.expectError(error.InvalidState, borrowed.session.getSnapshot(null));
    }
    try fixture.releaseRuntimeWhenChildless();
}
