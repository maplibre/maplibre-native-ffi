const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");

pub const style_json =
    \\{
    \\  "version": 8,
    \\  "name": "zig-binding-test",
    \\  "sources": {
    \\    "point": {
    \\      "type": "geojson",
    \\      "data": {
    \\        "type": "FeatureCollection",
    \\        "features": [
    \\          {"type":"Feature","id":"feature-1","geometry":{"type":"Point","coordinates":[-122.4194,37.7749]},"properties":{"visible":true,"kind":"capital"}}
    \\        ]
    \\      }
    \\    }
    \\  },
    \\  "layers": [
    \\    {"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}},
    \\    {"id":"point-circle","type":"circle","source":"point","paint":{"circle-color":"#f97316","circle-radius":12}}
    \\  ]
    \\}
;

/// Pumps the runtime and waits for an event of `event_type`, reporting whether
/// it arrived.
///
/// A wait that stops at the event it looks for takes one event per drain, so the
/// events a later wait needs stay queued rather than being dropped with the
/// batch that carried them.
pub fn waitForEvent(runtime: *maplibre.RuntimeHandle, event_type: maplibre.RuntimeEventType) !bool {
    for (0..1000) |_| {
        try runtime.pump(0);
        while (true) {
            var batch = try runtime.drainEvents(testing.allocator, 1);
            defer batch.deinit();
            if (batch.len() == 0) break;
            const event = try batch.at(0);
            if (std.meta.eql(event.event_type, event_type)) return true;
        }
        try sleepOneMillisecond();
    }
    return false;
}

/// Waits for an event of `event_type` and copies it out of its batch. The caller
/// deinits the returned event. See `waitForEvent` for what a bounded drain keeps
/// queued.
pub fn waitForOwnedEvent(
    runtime: *maplibre.RuntimeHandle,
    event_type: maplibre.RuntimeEventType,
) !maplibre.RuntimeEvent {
    for (0..5000) |_| {
        try runtime.pump(0);
        while (true) {
            var batch = try runtime.drainEvents(testing.allocator, 1);
            defer batch.deinit();
            if (batch.len() == 0) break;
            const event = try batch.at(0);
            if (std.meta.eql(event.event_type, event_type)) return try event.clone(testing.allocator);
        }
        try sleepOneMillisecond();
    }
    return error.EventNotObserved;
}

/// Drains every queued event, reporting how many the batch carried.
pub fn drainEvents(runtime: *maplibre.RuntimeHandle) !usize {
    var batch = try runtime.drainEvents(testing.allocator, 0);
    defer batch.deinit();
    return batch.len();
}

/// Creates a map with `style_json` loaded.
pub fn createLoadedMap(runtime: *maplibre.RuntimeHandle) !maplibre.MapHandle {
    var map = try maplibre.MapHandle.create(runtime, .{});
    errdefer map.close() catch {};
    try map.setStyleJson(testing.allocator, style_json);
    try testing.expect(try waitForEvent(runtime, .map_style_loaded));
    return map;
}

fn sleepOneMillisecond() !void {
    try testing.io.sleep(.fromMilliseconds(1), .awake);
}

pub fn typeNameContains(comptime T: type, comptime needle: []const u8) bool {
    if (std.mem.indexOf(u8, @typeName(T), needle) != null) return true;
    const info = @typeInfo(T);
    if (info != .@"struct") return false;
    inline for (info.@"struct".fields) |field| {
        if (std.mem.indexOf(u8, @typeName(field.type), needle) != null) return true;
    }
    return false;
}
