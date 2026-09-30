//! Owner handles: copies, closes, parents, and the futures that create them.

const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("fixture.zig");

// A handle value is a copy of an ID in the owner registry, so every copy sees
// the close that one of them performed, and a second close does nothing.
test "copied handles share one close and close twice safely" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    const map_copy = fixture.map;
    const runtime_copy = fixture.runtime;
    const projection = try support.resolve(try maplibre.mapProjectionCreate(fixture.map, null));
    const projection_copy = projection;

    try maplibre.mapProjectionClose(projection, null);
    try maplibre.mapProjectionClose(projection_copy, null);
    try testing.expectError(error.InvalidState, maplibre.mapProjectionGetCamera(projection_copy, null));

    try fixture.closeMap();
    try support.closeMapHandle(map_copy);
    var diagnostic: maplibre.Diagnostic = .{};
    try testing.expectError(error.InvalidState, maplibre.mapSnapshotGet(map_copy, &diagnostic));
    try testing.expectEqual(@as(?i32, null), diagnostic.raw_status);

    try support.closeRuntime(runtime_copy);
    fixture.runtime_open = false;
    try support.closeRuntime(fixture.runtime);
    try testing.expectError(error.InvalidState, maplibre.runtimeDrainEvents(runtime_copy, null));
}

// Native refuses to release a runtime with a live map. The refusal rolls the
// binding's close back, so the runtime keeps working and closes later.
test "a refused close leaves the handle usable and a later close succeeds" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();

    var diagnostic: maplibre.Diagnostic = .{};
    try testing.expectError(error.InvalidState, maplibre.runtimeRelease(fixture.runtime, &diagnostic));
    try testing.expectEqual(@as(?i32, -2), diagnostic.raw_status);
    try fixture.barrier();
    try fixture.closeMap();
}

fn ignoreWake(_: ?*anyopaque) maplibre.Error!void {}

fn countRelease(context: ?*anyopaque) void {
    const counter: *support.Counter = @ptrCast(@alignCast(context.?));
    counter.add();
}

// A map anchors its runtime, and native retirement of a disposed runtime waits
// for its live maps, so the runtime's callback registrations stay rooted until
// the last map goes.
test "a child map keeps its disposed runtime's callbacks alive" {
    var wake_releases = support.Counter{};
    var options = try maplibre.runtimeOptionsDefault(testing.allocator);
    defer options.deinit();
    options.value.event_wake = .{ .callback = ignoreWake, .context = &wake_releases, .release_context = countRelease };
    var runtime = try maplibre.runtimeCreate(testing.allocator, options.value, null);
    var map = try support.resolve(try maplibre.mapCreate(testing.allocator, runtime, try maplibre.mapOptionsDefault(), null));

    runtime.deinit();
    try testing.expectError(error.InvalidState, maplibre.runtimeGetEventMask(runtime, null));
    _ = try maplibre.mapSnapshotGet(map, null);
    try testing.expectEqual(@as(usize, 0), wake_releases.get());

    map.deinit();
    try wake_releases.waitFor(1);
}

// A creation future owns the object it adopts until a wait hands it out, so
// discarding the future, before or after native completes it, retires the map.
test "discarding a creation future retires the map it creates" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    try fixture.closeMap();

    var completed = try maplibre.mapCreate(testing.allocator, fixture.runtime, try maplibre.mapOptionsDefault(), null);
    while (!try completed.poll()) try fixture.barrier();
    completed.deinit();
    var pending = try maplibre.mapCreate(testing.allocator, fixture.runtime, try maplibre.mapOptionsDefault(), null);
    pending.deinit();
    try fixture.barrier();

    // Both maps retire, so the runtime is left with no child to refuse its
    // release.
    try fixture.releaseRuntimeWhenChildless();
}

// An install submits under its own lease of the prepared data, so releasing
// the data right after submission leaves the installs intact, and the released
// handle is then stale for new installs.
test "prepared GeoJSON data outlives its release through submitted installs" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    try fixture.loadStyle();

    const data = try maplibre.geojsonSourceDataCreate(testing.allocator, "{\"type\":\"FeatureCollection\",\"features\":[]}", null, null);
    const first = try maplibre.mapAddGeojsonSourceData(fixture.map, "first", data, null);
    const second = try maplibre.mapAddGeojsonSourceData(fixture.map, "second", data, null);
    try maplibre.geojsonSourceDataDestroy(data);
    try maplibre.geojsonSourceDataDestroy(data);
    try support.expectCommitted(first);
    try support.expectCommitted(second);
    var ids = try support.resolve(try maplibre.mapListStyleSourceIds(testing.allocator, fixture.map, null));
    defer ids.deinit();
    try testing.expectEqual(@as(usize, 2), ids.value.len);

    try testing.expectError(error.InvalidState, maplibre.mapAddGeojsonSourceData(fixture.map, "third", data, null));
}
