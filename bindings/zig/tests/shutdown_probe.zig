//! Returns from main with a runtime, a map, a resource provider, a runtime
//! wake, and the process-global log callback all live. The test step runs this
//! executable and fails unless the process exits with status 0, so native
//! teardown at process exit has to tolerate what a host abandons.

const std = @import("std");
const maplibre = @import("maplibre_native_ffi");

fn consumeLog(_: ?*anyopaque, _: maplibre.LogSeverity, _: maplibre.LogEvent, _: i64, _: []const u8) maplibre.Error!bool {
    return true;
}

fn passThrough(_: ?*anyopaque, _: maplibre.ResourceRequestInfo, request: maplibre.ResourceRequest) maplibre.Error!void {
    var owned = request;
    try owned.passThrough(null);
}

fn resolve(future_value: anytype) !@TypeOf(future_value).Value {
    var future = future_value;
    defer future.deinit();
    return future.wait(null);
}

pub fn main() !void {
    const allocator = std.heap.smp_allocator;
    try maplibre.logSetCallback(.{ .callback = consumeLog }, null);

    var options = try maplibre.runtimeOptionsDefault(allocator);
    defer options.deinit();
    const runtime = try maplibre.runtimeCreate(options.value, null);
    _ = try resolve(try runtime.setResourceProvider(allocator, .{ .callback = passThrough }, null));
    const created = try runtime.createMap(maplibre.mapOptionsDefault(), null);
    const map = created.map;
    try resolve(created.ready);
    _ = try resolve(try map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}", null));
    _ = try resolve(try runtime.clearResourceTransform(null));
}
