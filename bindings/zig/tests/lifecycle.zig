//! Owner handles: copies, native retirement, and the futures that create them.

const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("fixture.zig");

// Every copy carries the same native id, which native validates after release.
// The closed state belongs to the value that a release closed, so a copy stays
// open and its own release reaches native.
test "copied handles report native invalid state after release" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    var map_copy = fixture.map;
    var runtime_copy = fixture.runtime;
    var projection = try maplibre.mapProjectionCreate(testing.allocator, (try maplibre.mapGetSnapshot(fixture.map, null)).view, null);
    var projection_copy = projection;

    maplibre.mapProjectionRelease(&projection);
    try testing.expect(!projection_copy.isClosed());
    try testing.expectError(error.InvalidState, maplibre.mapProjectionGetCamera(projection_copy, null));
    // A void release reports nothing for a stale handle.
    maplibre.mapProjectionRelease(&projection_copy);

    try fixture.closeMap();
    try testing.expectError(error.InvalidState, support.closeMapHandle(&map_copy));
    try testing.expect(!map_copy.isClosed());
    var diagnostic: maplibre.Diagnostic = .{};
    try testing.expectError(error.InvalidState, maplibre.mapGetSnapshot(map_copy, &diagnostic));
    try testing.expectEqual(.invalid_state, diagnostic.status);
    try testing.expect(diagnostic.message().len != 0);

    try support.closeRuntimeHandle(&fixture.runtime);
    try testing.expectError(error.InvalidState, support.closeRuntimeHandle(&runtime_copy));
    try testing.expectError(error.InvalidState, maplibre.runtimeGetEventMask(runtime_copy, null));
}

// A handle's id is the source that its events carry, and a close leaves it in
// place, so a host can route events that it drains later.
test "a handle's raw value names its events and outlives close" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    try support.expectCommitted(try maplibre.mapSetStyleJson(fixture.map, support.style_json, null));
    var loaded = try fixture.waitForEvent(.map_style_loaded);
    defer loaded.deinit();
    try testing.expectEqual(fixture.map.id(), loaded.value.source);
    try fixture.closeMap();
    try testing.expect(fixture.map.isClosed());
    try testing.expectEqual(loaded.value.source, fixture.map.id());
}

// A dispose closes the value it is given. A second dispose and a later deinit
// do nothing, while an asynchronous release still reaches native.
test "a disposed handle is closed and a second dispose does nothing" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    try testing.expect(!fixture.map.isClosed());
    try expectFormat("Map(id=0x{x}, closed=false)", fixture.map);

    try maplibre.mapDispose(&fixture.map, null);
    try testing.expect(fixture.map.isClosed());
    try maplibre.mapDispose(&fixture.map, null);
    fixture.map.deinit();
    try expectFormat("Map(id=0x{x}, closed=true)", fixture.map);
    try testing.expectError(error.InvalidState, maplibre.mapRelease(&fixture.map, null));

    // A runtime accepts its dispose while its disposed map retires.
    try maplibre.runtimeDispose(&fixture.runtime, null);
    try testing.expect(fixture.runtime.isClosed());
    try maplibre.runtimeDispose(&fixture.runtime, null);
    fixture.runtime.deinit();
}

// A handle whose dispose is its void release closes on either call.
test "a released projection is closed and a second release or deinit does nothing" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    var projection = try maplibre.mapProjectionCreate(testing.allocator, (try maplibre.mapGetSnapshot(fixture.map, null)).view, null);
    try testing.expect(!projection.isClosed());
    try testing.expect(projection.id() != 0);

    maplibre.mapProjectionRelease(&projection);
    try testing.expect(projection.isClosed());
    maplibre.mapProjectionRelease(&projection);
    projection.deinit();
    try expectFormat("MapProjection(id=0x{x}, closed=true)", projection);

    var scoped = try maplibre.mapProjectionCreate(testing.allocator, (try maplibre.mapGetSnapshot(fixture.map, null)).view, null);
    scoped.deinit();
    try testing.expect(scoped.isClosed());
    scoped.deinit();
    maplibre.mapProjectionRelease(&scoped);
}

/// Checks a handle's `{f}` text against `template`, which formats its id.
fn expectFormat(comptime template: []const u8, handle: anytype) !void {
    var expected: [64]u8 = undefined;
    var actual: [64]u8 = undefined;
    try testing.expectEqualStrings(
        try std.fmt.bufPrint(&expected, template, .{handle.id()}),
        try std.fmt.bufPrint(&actual, "{f}", .{handle}),
    );
}

// Native refuses to release a runtime with a live map, which leaves its id usable.
test "a refused close leaves the handle usable and a later close succeeds" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();

    var diagnostic: maplibre.Diagnostic = .{};
    try testing.expectError(error.InvalidState, maplibre.runtimeRelease(&fixture.runtime, &diagnostic));
    try testing.expect(!fixture.runtime.isClosed());
    try testing.expectEqual(.invalid_state, diagnostic.status);
    try fixture.fence();
    try fixture.closeMap();
}

fn ignoreEvents(_: *support.Counter, _: maplibre.EventBatchView) void {}

fn countRelease(counter: *support.Counter) void {
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
    try support.resolve(try maplibre.runtimeSetEventHandler(testing.allocator, runtime, .init(&handler_releases, ignoreEvents, countRelease), null));
    var map = try support.resolve(try maplibre.runtimeCreateMap(testing.allocator, runtime, maplibre.mapOptionsDefault(), null));

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

    var completed = try maplibre.runtimeCreateMap(testing.allocator, fixture.runtime, maplibre.mapOptionsDefault(), null);
    while (!try completed.poll()) try fixture.fence();
    completed.deinit();
    var pending = try maplibre.runtimeCreateMap(testing.allocator, fixture.runtime, maplibre.mapOptionsDefault(), null);
    pending.deinit();
    try fixture.fence();

    // Both maps retire, so the runtime is left with no child to refuse its
    // release.
    try fixture.releaseRuntimeWhenChildless();
}
