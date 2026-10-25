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
    var projection = try maplibre.mapProjectionCreate((try fixture.map.getSnapshot(null)).view, null);
    var projection_copy = projection;

    projection.release();
    try testing.expect(!projection_copy.isClosed());
    try testing.expectError(error.InvalidState, projection_copy.getCamera(null));
    // A void release reports nothing for a stale handle.
    projection_copy.release();

    try fixture.closeMap();
    try testing.expectError(error.InvalidState, support.closeMapHandle(&map_copy));
    try testing.expect(!map_copy.isClosed());
    var diagnostic: maplibre.Diagnostic = .{};
    try testing.expectError(error.InvalidState, map_copy.getSnapshot(&diagnostic));
    try testing.expectEqual(.invalid_state, diagnostic.status);
    try testing.expect(diagnostic.message().len != 0);

    try support.closeRuntimeHandle(&fixture.runtime);
    try testing.expectError(error.InvalidState, support.closeRuntimeHandle(&runtime_copy));
    try testing.expectError(error.InvalidState, runtime_copy.getEventMask(null));
}

// A handle's id is the source that its events carry, and a close leaves it in
// place, so a host can route events that it drains later.
test "a handle's raw value names its events and outlives close" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    try support.expectCommitted(try fixture.map.setStyleJson(support.style_json, null));
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

    try fixture.map.dispose(null);
    try testing.expect(fixture.map.isClosed());
    try fixture.map.dispose(null);
    fixture.map.deinit();
    try expectFormat("Map(id=0x{x}, closed=true)", fixture.map);
    try testing.expectError(error.InvalidState, fixture.map.release(null));

    // A runtime accepts its dispose while its disposed map retires.
    try fixture.runtime.dispose(null);
    try testing.expect(fixture.runtime.isClosed());
    try fixture.runtime.dispose(null);
    fixture.runtime.deinit();
}

// A handle whose dispose is its void release closes on either call.
test "a released projection is closed and a second release or deinit does nothing" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    var projection = try maplibre.mapProjectionCreate((try fixture.map.getSnapshot(null)).view, null);
    try testing.expect(!projection.isClosed());
    try testing.expect(projection.id() != 0);

    projection.release();
    try testing.expect(projection.isClosed());
    projection.release();
    projection.deinit();
    try expectFormat("MapProjection(id=0x{x}, closed=true)", projection);

    var scoped = try maplibre.mapProjectionCreate((try fixture.map.getSnapshot(null)).view, null);
    scoped.deinit();
    try testing.expect(scoped.isClosed());
    scoped.deinit();
    scoped.release();
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
    try testing.expectError(error.InvalidState, fixture.runtime.release(&diagnostic));
    try testing.expect(!fixture.runtime.isClosed());
    try testing.expectEqual(.invalid_state, diagnostic.status);
    try fixture.fence();
    try fixture.closeMap();
}

fn ignoreEvents(_: *support.Counter, _: []const maplibre.RuntimeEvent) void {}

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
    var runtime = try maplibre.runtimeCreate(options.value, null);
    try support.expectCommitted(try runtime.setEventHandler(.init(&handler_releases, ignoreEvents, countRelease), null));
    const created = try runtime.createMap(maplibre.mapOptionsDefault(), null);
    var map = created.map;
    try support.resolve(created.ready);

    runtime.deinit();
    try testing.expectError(error.InvalidState, runtime.getEventMask(null));
    _ = try map.getSnapshot(null);
    // No fence can follow the disposal: native refuses the map's commands
    // once its runtime is disposed. The count stays zero without one because
    // native releases the handler only when it retires the runtime, which this
    // live map holds off, and the binding never releases it on its own.
    try testing.expectEqual(@as(usize, 0), handler_releases.get());

    map.deinit();
    try handler_releases.waitFor(1);
}
