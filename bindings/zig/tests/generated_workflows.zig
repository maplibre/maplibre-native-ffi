const std = @import("std");
const testing = std.testing;
const maplibre = @import("maplibre_native_ffi");
const support = @import("fixture.zig");

test "discarded commands are fenced and synchronous rejection returns an error" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    try fixture.map.discarding().setDebugOptions(.{ .tile_borders = true }, null);
    // A later command on the same map completes after the discarded one.
    _ = try support.resolve(try fixture.map.requestRepaint(null));
    const snapshot = try fixture.map.getSnapshot(null);
    try testing.expect(snapshot.debug_options.tile_borders);
    try testing.expectError(error.InvalidArgument, fixture.map.discarding().resize(.{ .width = 0, .height = 8, .scale_factor = 1 }, null));
}

const ProviderProbe = struct {
    runtime: maplibre.Runtime,
    calls: std.atomic.Value(usize) = .init(0),
    releases: std.atomic.Value(usize) = .init(0),
    fn provide(self: *@This(), _: maplibre.ResourceRequestInfo, handle: maplibre.ResourceRequest) maplibre.Error!void {
        _ = self.calls.fetchAdd(1, .seq_cst);
        var owned = handle;
        defer owned.deinit();
        var cleared = try self.runtime.clearResourceTransform(null);
        cleared.deinit();
    }
    fn release(self: *@This()) void {
        _ = self.releases.fetchAdd(1, .seq_cst);
    }
};

// A provider may submit a command and release its request without answering it
// while its callback runs, which fails the request.
test "a provider can deinit its request inside its callback" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    var probe = ProviderProbe{ .runtime = fixture.runtime };
    try fixture.setProvider(.init(&probe, ProviderProbe.provide, ProviderProbe.release));
    _ = try support.resolve(try fixture.map.setStyleUrl("binding-test://style", null));
    var failed = try fixture.waitForEvent(.map_loading_failed);
    failed.deinit();
    try support.resolve(try fixture.runtime.clearResourceProvider(null));
    try testing.expectEqual(@as(usize, 1), probe.calls.load(.seq_cst));
    try testing.expectEqual(@as(usize, 1), probe.releases.load(.seq_cst));
}
