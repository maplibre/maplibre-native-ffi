const std = @import("std");
const testing = std.testing;
const maplibre = @import("maplibre_native_ffi");
const support = @import("fixture.zig");

test "discarded commands are fenced and synchronous rejection returns an error" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    try fixture.map.discarding().setDebugOptions(.{ .tile_borders = true }, null);
    // A later command on the same map completes after the discarded one.
    _ = try support.resolve(try maplibre.mapRequestRepaint(fixture.map, null));
    const snapshot = try maplibre.mapGetSnapshot(fixture.map, null);
    try testing.expect(snapshot.debug_options.tile_borders);
    try testing.expectError(error.InvalidArgument, fixture.map.discarding().resize(.{ .width = 0, .height = 8, .scale_factor = 1 }, null));
}

const ProviderProbe = struct {
    runtime: maplibre.Runtime,
    calls: std.atomic.Value(usize) = .init(0),
    releases: std.atomic.Value(usize) = .init(0),
    fn provide(self: *@This(), request: maplibre.ResourceRequestInfo, handle: maplibre.ResourceRequest) maplibre.Error!maplibre.ResourceProviderDecision {
        _ = self.calls.fetchAdd(1, .seq_cst);
        if (request.kind != .style) return .pass_through;
        var cleared = try maplibre.runtimeClearResourceTransform(self.runtime, null);
        cleared.deinit();
        try maplibre.resourceRequestComplete(std.heap.smp_allocator, handle, .{ .bytes = support.style_json }, null);
        var owned = handle;
        owned.deinit();
        return .pass_through;
    }
    fn release(self: *@This()) void {
        _ = self.releases.fetchAdd(1, .seq_cst);
    }
};

test "a provider can deinit its request inside its callback" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    var probe = ProviderProbe{ .runtime = fixture.runtime };
    try fixture.setProvider(.init(&probe, ProviderProbe.provide, ProviderProbe.release));
    _ = try support.resolve(try maplibre.mapSetStyleUrl(fixture.map, "binding-test://style", null));
    var loaded = try fixture.waitForEvent(.map_style_loaded);
    loaded.deinit();
    try support.resolve(try maplibre.runtimeClearResourceProvider(fixture.runtime, null));
    try testing.expectEqual(@as(usize, 1), probe.calls.load(.seq_cst));
    try testing.expectEqual(@as(usize, 1), probe.releases.load(.seq_cst));
}
