//! Owner handles: copies, native retirement, and the futures that create them.

const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("fixture.zig");

// Every copy carries the same native id, which native validates after release.
test "copied handles report native invalid state after release" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    const map_copy = fixture.map;
    const runtime_copy = fixture.runtime;
    const projection = try maplibre.mapProjectionCreate(testing.allocator, (try maplibre.mapGetSnapshot(fixture.map, null)).view, null);
    const projection_copy = projection;

    try maplibre.mapProjectionRelease(projection, null);
    try testing.expectError(error.InvalidState, maplibre.mapProjectionRelease(projection_copy, null));
    try testing.expectError(error.InvalidState, maplibre.mapProjectionGetCamera(projection_copy, null));

    try fixture.closeMap();
    try testing.expectError(error.InvalidState, support.closeMapHandle(map_copy));
    var diagnostic: maplibre.Diagnostic = .{};
    try testing.expectError(error.InvalidState, maplibre.mapGetSnapshot(map_copy, &diagnostic));
    try testing.expectEqual(@as(?i32, -2), diagnostic.raw_status);
    try testing.expect(diagnostic.message().len != 0);

    try support.closeRuntimeHandle(runtime_copy);
    fixture.runtime_open = false;
    try testing.expectError(error.InvalidState, support.closeRuntimeHandle(fixture.runtime));
    try testing.expectError(error.InvalidState, maplibre.runtimeGetEventMask(runtime_copy, null));
}

// A handle's raw value is the source that its events carry, and a close leaves
// it in place, so a host can route events that it drains later.
test "a handle's raw value names its events and outlives close" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    try support.expectCommitted(try maplibre.mapSetStyleJson(fixture.map, support.style_json, null));
    var loaded = try fixture.waitForEvent(.map_style_loaded);
    defer loaded.deinit();
    try testing.expectEqual(fixture.map.raw, loaded.value.source);
    const map = fixture.map;
    try fixture.closeMap();
    try testing.expectEqual(loaded.value.source, map.raw);
}

// Native refuses to release a runtime with a live map, which leaves its id usable.
test "a refused close leaves the handle usable and a later close succeeds" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();

    var diagnostic: maplibre.Diagnostic = .{};
    try testing.expectError(error.InvalidState, maplibre.runtimeRelease(fixture.runtime, &diagnostic));
    try testing.expectEqual(@as(?i32, -2), diagnostic.raw_status);
    try fixture.fence();
    try fixture.closeMap();
}

fn ignoreEvents(_: ?*anyopaque, _: maplibre.EventBatchView) maplibre.Error!void {}

fn countRelease(context: ?*anyopaque) void {
    const counter: *support.Counter = @ptrCast(@alignCast(context.?));
    counter.add();
}

// Native retirement of a disposed runtime waits
// for its live maps, so the runtime's callback registrations stay rooted until
// the last map goes.
test "a child map keeps its disposed runtime's callbacks alive" {
    var handler_releases = support.Counter{};
    var options = try maplibre.runtimeOptionsDefault(testing.allocator);
    defer options.deinit();
    var runtime = try maplibre.runtimeCreate(testing.allocator, options.value, null);
    try support.resolve(try maplibre.runtimeSetEventHandler(testing.allocator, runtime, .{
        .callback = ignoreEvents,
        .context = &handler_releases,
        .release_context = countRelease,
    }, null));
    var map = try support.resolve(try maplibre.runtimeCreateMap(testing.allocator, runtime, try maplibre.mapOptionsDefault(), null));

    runtime.deinit();
    try testing.expectError(error.InvalidState, maplibre.runtimeGetEventMask(runtime, null));
    _ = try maplibre.mapGetSnapshot(map, null);
    // No fence can follow the disposal: native refuses the map's commands
    // once its runtime is disposed. The count stays zero without one because
    // native releases the handler only when it retires the runtime, which this
    // live map holds off, and the binding never releases it on its own.
    try testing.expectEqual(@as(usize, 0), handler_releases.get());

    map.deinit();
    try handler_releases.waitFor(1);
}

// A creation future owns the object it adopts until a wait hands it out, so
// discarding the future, before or after native completes it, retires the map.
test "discarding a creation future retires the map it creates" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    try fixture.closeMap();

    var completed = try maplibre.runtimeCreateMap(testing.allocator, fixture.runtime, try maplibre.mapOptionsDefault(), null);
    while (!try completed.poll()) try fixture.fence();
    completed.deinit();
    var pending = try maplibre.runtimeCreateMap(testing.allocator, fixture.runtime, try maplibre.mapOptionsDefault(), null);
    pending.deinit();
    try fixture.fence();

    // Both maps retire, so the runtime is left with no child to refuse its
    // release.
    try fixture.releaseRuntimeWhenChildless();
}
