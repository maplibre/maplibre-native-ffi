const std = @import("std");
const testing = std.testing;
const maplibre = @import("maplibre_native_ffi");
const support = @import("fixture.zig");

test "generated owners preserve rejected release and copied event snapshots" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    try testing.expectError(error.InvalidState, maplibre.runtimeRelease(fixture.runtime, null));
    const command = try support.resolve(try maplibre.mapSetStyleJson(fixture.map, support.style_json, null));
    try command.statusError();
    try fixture.barrier();
    var batch = try maplibre.runtimeDrainEvents(fixture.runtime, null) orelse return error.EventNotObserved;
    defer batch.deinit();
    var copied = try maplibre.eventBatchGet(testing.allocator, batch, null);
    defer copied.deinit();
    try maplibre.eventBatchRelease(batch);
    try testing.expect(copied.value.events.len != 0);
    const saved = fixture.map;
    try fixture.closeMap();
    try testing.expectError(error.InvalidState, maplibre.mapSnapshotGet(saved, null));
}

const ProviderProbe = struct {
    runtime: maplibre.Runtime,
    calls: std.atomic.Value(usize) = .init(0),
    releases: std.atomic.Value(usize) = .init(0),
    fn provide(context: ?*anyopaque, request: maplibre.ResourceRequest, handle: maplibre.ResourceRequestHandle) maplibre.Error!maplibre.ResourceProviderDecision {
        const self: *@This() = @ptrCast(@alignCast(context.?));
        _ = self.calls.fetchAdd(1, .seq_cst);
        if (request.kind != .style) return .pass_through;
        if (maplibre.runtimeBarrier(self.runtime, null)) |unexpected| {
            var future = unexpected;
            future.deinit();
            return error.NativeError;
        } else |err| {
            if (err != error.InvalidState) return err;
        }
        try maplibre.resourceRequestComplete(std.heap.smp_allocator, handle, .{ .bytes = support.style_json }, null);
        try maplibre.resourceRequestRelease(handle);
        try maplibre.resourceRequestRelease(handle);
        return .pass_through;
    }
    fn release(context: ?*anyopaque) void {
        const self: *@This() = @ptrCast(@alignCast(context.?));
        _ = self.releases.fetchAdd(1, .seq_cst);
    }
};

test "generated provider inline completion and close retain decision ownership" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    var probe = ProviderProbe{ .runtime = fixture.runtime };
    try fixture.setProvider(.{ .context = &probe, .callback = ProviderProbe.provide, .release_context = ProviderProbe.release });
    try (try support.resolve(try maplibre.mapSetStyleUrl(testing.allocator, fixture.map, "binding-test://style", null))).statusError();
    var loaded = try fixture.waitForEvent(.map_style_loaded);
    loaded.deinit();
    try support.resolve(try maplibre.runtimeClearResourceProvider(fixture.runtime, null));
    try testing.expectEqual(@as(usize, 1), probe.calls.load(.seq_cst));
    try testing.expectEqual(@as(usize, 1), probe.releases.load(.seq_cst));
}
