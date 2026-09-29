const std = @import("std");
const testing = std.testing;
const g = @import("maplibre_native_ffi");
fn resolve(future_value: anytype) !@TypeOf(future_value).Value {
    var future = future_value;
    defer future.deinit();
    return future.wait(null);
}
const test_style = "{\"version\":8,\"sources\":{},\"layers\":[]}";

test "generated owners preserve rejected release and copied event snapshots" {
    var options = try g.runtimeOptionsDefault(testing.allocator);
    defer options.deinit();
    var runtime = try g.runtimeCreate(testing.allocator, options.value, null);
    defer runtime.deinit();
    var map = try resolve(try g.mapCreate(testing.allocator, runtime, try g.mapOptionsDefault()));
    defer map.deinit();
    try testing.expectError(error.InvalidState, g.runtimeRelease(runtime));
    const command = try resolve(try g.mapSetStyleJson(map, test_style));
    try command.statusError();
    try resolve(try g.runtimeBarrier(runtime));
    var batch = try g.runtimeDrainEvents(runtime);
    defer batch.deinit();
    var copied = try g.eventBatchGet(testing.allocator, batch);
    defer copied.deinit();
    try g.eventBatchRelease(batch);
    try testing.expect(copied.value.events.len != 0);
    const saved = map;
    try resolve(try g.mapRelease(map));
    try testing.expectError(error.InvalidState, g.mapSnapshotGet(saved));
    try resolve(try g.runtimeRelease(runtime));
}

const ProviderProbe = struct {
    runtime: g.Runtime,
    calls: std.atomic.Value(usize) = .init(0),
    releases: std.atomic.Value(usize) = .init(0),
    fn provide(context: ?*anyopaque, request: g.ResourceRequest, handle: g.ResourceRequestHandle) g.Error!g.ResourceProviderDecision {
        const self: *@This() = @ptrCast(@alignCast(context.?));
        _ = self.calls.fetchAdd(1, .seq_cst);
        if (request.kind != .style) return .pass_through;
        if (g.runtimeBarrier(self.runtime)) |unexpected| {
            var future = unexpected;
            future.deinit();
            return error.NativeError;
        } else |err| {
            if (err != error.InvalidState) return err;
        }
        try g.resourceRequestComplete(std.heap.smp_allocator, handle, .{ .bytes = test_style });
        try g.resourceRequestRelease(handle);
        try g.resourceRequestRelease(handle);
        return .pass_through;
    }
    fn release(context: ?*anyopaque) void {
        const self: *@This() = @ptrCast(@alignCast(context.?));
        _ = self.releases.fetchAdd(1, .seq_cst);
    }
};

test "generated provider inline completion and close retain decision ownership" {
    var options = try g.runtimeOptionsDefault(testing.allocator);
    defer options.deinit();
    var runtime = try g.runtimeCreate(testing.allocator, options.value, null);
    defer runtime.deinit();
    var map = try resolve(try g.mapCreate(testing.allocator, runtime, try g.mapOptionsDefault()));
    defer map.deinit();
    var probe = ProviderProbe{ .runtime = runtime };
    try resolve(try g.runtimeSetResourceProvider(testing.allocator, runtime, .{ .context = &probe, .callback = ProviderProbe.provide, .release_context = ProviderProbe.release }));
    try (try resolve(try g.mapSetStyleUrl(testing.allocator, map, "binding-test://style"))).statusError();
    var loaded = false;
    for (0..1000) |_| {
        var batch = try g.runtimeDrainEvents(runtime);
        defer batch.deinit();
        var copy = try g.eventBatchGet(testing.allocator, batch);
        defer copy.deinit();
        for (copy.value.events) |event| if (event.type == .map_style_loaded) {
            loaded = true;
        };
        if (loaded) break;
        try testing.io.sleep(.fromMilliseconds(1), .awake);
    }
    try testing.expect(loaded);
    try resolve(try g.runtimeClearResourceProvider(runtime));
    try testing.expectEqual(@as(usize, 1), probe.calls.load(.seq_cst));
    try testing.expectEqual(@as(usize, 1), probe.releases.load(.seq_cst));
    try resolve(try g.mapRelease(map));
    try resolve(try g.runtimeRelease(runtime));
}
