//! Render sessions on the build's backend, with GPU contexts from
//! tests/graphics.

const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("fixture.zig");

const extent = maplibre.LogicalExtent{ .width = 32, .height = 16, .scale_factor = 1.0 };

fn createFixture() !*support.Fixture {
    const fixture = try support.Fixture.create(.{ .extent = .{ .width = extent.width, .height = extent.height, .scale_factor = extent.scale_factor } });
    errdefer fixture.destroy();
    try fixture.loadStyle();
    return fixture;
}

/// Renders one frame, reads the texture back, and checks that the middle
/// pixel holds the style's background color.
fn renderAndReadBack(owned: *support.OwnedTexture) !void {
    const capabilities = try maplibre.renderSessionGetCapabilities(owned.session, null);
    try testing.expect(capabilities.flags.readback);
    _ = try owned.renderUntilRendered();
    var image = try owned.resolve(try maplibre.textureReadPremultipliedRgba8(testing.allocator, owned.session, null));
    defer image.deinit();
    try testing.expectEqual(extent.width, image.value.info.width);
    try testing.expectEqual(extent.height, image.value.info.height);
    try testing.expectEqual(image.value.info.byte_length, image.value.data.len);
    const offset = (image.value.info.height / 2) * image.value.info.stride + (image.value.info.width / 2) * 4;
    for (support.background_rgba, image.value.data[offset..][0..4]) |expected, actual| {
        try testing.expect(@abs(@as(i16, expected) - @as(i16, actual)) <= 2);
    }
}

test "an owned-texture session renders a frame that reads back as pixels" {
    const fixture = try createFixture();
    defer fixture.destroy();
    const owned = try support.OwnedTexture.attach(fixture.map, extent, support.OwnedTexture.default_driver);
    defer owned.close();
    try renderAndReadBack(owned);
}

const ThreadRender = struct {
    map: maplibre.Map,
    driver: ?maplibre.RenderDriverKind = null,
    result: anyerror!void = error.NotRun,

    fn run(self: *ThreadRender) void {
        self.result = self.render();
    }

    fn render(self: *ThreadRender) !void {
        const owned = try support.OwnedTexture.attach(self.map, extent, .caller_graphics_thread);
        defer owned.close();
        self.driver = (try maplibre.renderSessionGetCapabilities(owned.session, null)).driver;
        try renderAndReadBack(owned);
        try owned.detach();
    }
};

// A caller-driven session belongs to the thread that first services it. Zig
// has no managed event loop, so the thread is one the test spawns, and every
// session call, attachment through detach, runs there.
test "a caller-driven session is serviced from a thread the test spawns" {
    const fixture = try createFixture();
    defer fixture.destroy();
    var render = ThreadRender{ .map = fixture.map };
    const thread = try std.Thread.spawn(.{}, ThreadRender.run, .{&render});
    thread.join();
    try render.result;
    try testing.expectEqual(@as(?maplibre.RenderDriverKind, .caller_graphics_thread), render.driver);
}

// Before any demand, native reports the drain and the acquire as not ready,
// which reads as null.
test "a drain before any demand returns no batch" {
    const fixture = try createFixture();
    defer fixture.destroy();
    const owned = try support.OwnedTexture.attach(fixture.map, extent, support.OwnedTexture.default_driver);
    defer owned.close();
    try testing.expect(try maplibre.renderSessionDrainFrameResults(owned.session, null) == null);
    try testing.expect(try maplibre.renderSessionAcquireFrame(owned.session, null) == null);
}

// A scoped view leases its frame for the callback. Inside the scope, releasing
// that frame or abandoning its session reports busy, and a host error passes
// through. Disposing a sibling frame, which has no consumer fence, leaves the
// view readable. Once the scope returns, the frame releases normally.
test "a scoped frame view holds its frame until the scope returns" {
    const fixture = try createFixture();
    defer fixture.destroy();
    const owned = try support.OwnedTexture.attach(fixture.map, extent, support.OwnedTexture.default_driver);
    defer owned.close();
    _ = try owned.renderUntilRendered();
    var frame = (try maplibre.renderSessionAcquireFrame(owned.session, null)).?;
    defer frame.deinit();
    try support.expectCommitted(try maplibre.mapRequestRepaint(fixture.map, null));
    _ = try owned.renderUntilRendered();
    var sibling = (try maplibre.renderSessionAcquireFrame(owned.session, null)).?;
    defer sibling.deinit();

    const width = try support.getOwnedTexture(u32, frame, {}, struct {
        fn use(_: void, view: support.OwnedTextureFrame) anyerror!u32 {
            return view.width;
        }
    }.use);
    try testing.expectEqual(extent.width, width);

    const Probe = struct {
        frame: maplibre.AcquiredFrame,
        sibling: *maplibre.AcquiredFrame,
        session: maplibre.RenderSession,
        fn inspect(self: @This(), _: maplibre.GpuSync) anyerror!void {
            // The view borrows the frame, so the binding refuses its release,
            // and native refuses to abandon the session that owns it.
            var diagnostic: maplibre.Diagnostic = .{};
            try testing.expectError(error.InvalidState, maplibre.acquiredFrameRelease(testing.allocator, self.frame, .{ .kind = .cpu_complete }, &diagnostic));
            try testing.expectEqualStrings("AcquiredFrame is in use", diagnostic.message());
            try testing.expectError(error.Busy, maplibre.renderSessionAbandon(self.session, null));
            self.sibling.deinit();
            try maplibre.acquiredFrameGetProducerSync(void, self.frame, {}, struct {
                fn use(_: void, _: maplibre.GpuSync) anyerror!void {}
            }.use, null);
            return error.HostConsumerFailed;
        }
    };
    try testing.expectError(error.HostConsumerFailed, maplibre.acquiredFrameGetProducerSync(void, frame, Probe{ .frame = frame, .sibling = &sibling, .session = owned.session }, Probe.inspect, null));
    try maplibre.acquiredFrameRelease(testing.allocator, frame, .{ .kind = .cpu_complete }, null);
}

// A frame anchors its session and a session its map. Disposing the map first
// leaves its children to retire with it, and each dispose is safe in any order.
test "disposing a parent graph retires its acquired frame and session" {
    const fixture = try createFixture();
    defer fixture.destroy();
    const owned = try support.OwnedTexture.attach(fixture.map, extent, support.OwnedTexture.default_driver);
    defer owned.close();
    _ = try owned.renderUntilRendered();
    var frame = (try maplibre.renderSessionAcquireFrame(owned.session, null)).?;

    fixture.map.deinit();
    fixture.map_open = false;
    frame.deinit();
    owned.session.deinit();
    owned.attached = false;
    try testing.expectError(error.InvalidState, maplibre.acquiredFrameGetResult(frame, null));
    try testing.expectError(error.InvalidState, maplibre.renderSessionGetSnapshot(owned.session, null));
    // The graphics context outlives the session, so the test waits for the
    // runtime to retire everything before the deferred teardown destroys it.
    try fixture.releaseRuntimeWhenChildless();
}
