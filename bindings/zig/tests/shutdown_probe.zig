//! Returns from main with a runtime, a map, a resource provider, a runtime
//! wake, and the process-global log callback all live. The test step runs this
//! executable and fails unless the process exits with status 0, so native
//! teardown at process exit has to tolerate what a host abandons.

const std = @import("std");
const maplibre = @import("maplibre_native_ffi");

fn consumeLog(_: ?*anyopaque, _: maplibre.LogSeverity, _: maplibre.LogEvent, _: i64, _: []const u8) maplibre.Error!u32 {
    return 1;
}

fn ignoreWake(_: ?*anyopaque) maplibre.Error!void {}

fn passThrough(_: ?*anyopaque, _: maplibre.ResourceRequest, _: maplibre.ResourceRequestHandle) maplibre.Error!maplibre.ResourceProviderDecision {
    return .pass_through;
}

fn resolve(future_value: anytype) !@TypeOf(future_value).Value {
    var future = future_value;
    defer future.deinit();
    return future.wait(null);
}

pub fn main() !void {
    const allocator = std.heap.smp_allocator;
    try maplibre.validateAbiVersion(null);
    try maplibre.logSetCallback(.{ .call = consumeLog }, null);

    var options = try maplibre.runtimeOptionsDefault(allocator);
    defer options.deinit();
    options.value.event_wake = .{ .callback = ignoreWake };
    const runtime = try maplibre.runtimeCreate(allocator, options.value, null);
    try resolve(try maplibre.runtimeSetResourceProvider(allocator, runtime, .{ .callback = passThrough }, null));
    const map = try resolve(try maplibre.runtimeCreateMap(allocator, runtime, try maplibre.mapOptionsDefault(), null));
    const style = try resolve(try maplibre.mapSetStyleJson(map, "{\"version\":8,\"sources\":{},\"layers\":[]}", null));
    try style.statusError();
    try resolve(try maplibre.runtimeBarrier(runtime, null));
}
