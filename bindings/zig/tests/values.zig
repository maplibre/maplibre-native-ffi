//! The generated value shapes: strings, optional fields and masks, strided
//! batches and tagged unions, open enums and 64-bit carriers, and array inputs.

const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("fixture.zig");

// A C string field rejects an embedded NUL before any native call, while an
// explicit-length buffer carries the NUL to native unchanged.
test "strings cross as terminated and explicit-length views" {
    var options = try maplibre.runtimeOptionsDefault(testing.allocator);
    defer options.deinit();
    options.value.asset_path = "asset\x00path";
    var diagnostic: maplibre.Diagnostic = .{};
    try testing.expectError(error.InvalidString, maplibre.runtimeCreate(testing.allocator, options.value, &diagnostic));
    try testing.expectEqual(@as(?i32, null), diagnostic.raw_status);
    try testing.expectEqualStrings("the string contains a NUL byte", diagnostic.message());

    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    const rejected = try support.resolve(try maplibre.mapSetStyleJson(fixture.map, "{\x00}", null));
    try testing.expectEqual(maplibre.CommandDisposition.failed, rejected.disposition);
    try testing.expectError(error.InvalidArgument, rejected.statusError());

    try fixture.loadStyle();
    const source_id = "nul\x00source";
    try support.expectCommitted(try maplibre.mapAddStyleSourceJson(fixture.map, source_id, "{\"type\":\"geojson\",\"data\":{\"type\":\"FeatureCollection\",\"features\":[]}}", null));
    var info = (try support.resolve(try maplibre.mapGetStyleSource(testing.allocator, fixture.map, source_id, null))).?;
    defer info.deinit();
    try testing.expectEqual(maplibre.StyleSourceType.geojson, info.value.type);
    try testing.expect(try support.resolve(try maplibre.mapGetStyleSource(testing.allocator, fixture.map, "nul", null)) == null);
}

// A null optional field sets no presence bit, so the command leaves that value
// alone, and a packed mask round-trips bit for bit.
test "optional fields and masks round-trip through presence bits" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();

    try support.expectCommitted(try maplibre.mapUpdateCamera(testing.allocator, fixture.map, .{
        .mode = .jump,
        .camera = .{ .zoom = 3.0, .pitch = 30.0 },
    }, null));
    try support.expectCommitted(try maplibre.mapUpdateCamera(testing.allocator, fixture.map, .{
        .mode = .jump,
        .camera = .{ .zoom = 5.0 },
    }, null));
    const snapshot = try maplibre.mapGetCameraSnapshot(fixture.map, null);
    try testing.expectEqual(@as(?f64, 5.0), snapshot.camera.zoom);
    try testing.expectApproxEqAbs(@as(f64, 30.0), snapshot.camera.pitch.?, 1e-9);

    var mask = maplibre.RuntimeEventMask.all;
    mask.map_tile_action = false;
    mask.offline_region_status_changed = false;
    try maplibre.runtimeSetEventMask(fixture.runtime, mask, null);
    try testing.expectEqual(mask, try maplibre.runtimeGetEventMask(fixture.runtime, null));
}

// Native steps through a batch by the stride it reports, which may exceed the
// record size the binding was built with, and a payload tag the binding does
// not know decodes as `.unknown` with its raw value.
test "a strided event batch with an unknown payload arm decodes without loss" {
    const RawBatch = @typeInfo(@TypeOf(maplibre.EventBatchView.fromNative)).@"fn".params[1].type.?;
    const RawEvent = @typeInfo(@FieldType(RawBatch, "events")).pointer.child;
    const Record = extern struct { event: RawEvent, newer_fields: [24]u8 };

    const messages = "future\x00loaded\x00";
    var records = std.mem.zeroes([2]Record);
    records[0].event.type = 999;
    records[0].event.source = 0xfeed_0000_0000_0001;
    records[0].event.payload_type = 77;
    records[0].event.message_offset = 0;
    records[0].event.message_size = 6;
    records[0].newer_fields = @splat(0xa5);
    records[1].event.type = @intFromEnum(maplibre.RuntimeEventType.map_style_loaded);
    records[1].event.message_offset = 7;
    records[1].event.message_size = 6;
    var raw = std.mem.zeroes(RawBatch);
    raw.size = @sizeOf(RawBatch);
    raw.event_size = @sizeOf(Record);
    raw.events = &records[0].event;
    raw.event_count = records.len;
    raw.messages = messages.ptr;
    raw.messages_size = messages.len;

    var arena = std.heap.ArenaAllocator.init(testing.allocator);
    defer arena.deinit();
    const view = try maplibre.EventBatchView.fromNative(arena.allocator(), raw);
    try testing.expectEqual(@as(usize, 2), view.events.len);
    try testing.expectEqual(@as(u32, 999), @intFromEnum(view.events[0].type));
    try testing.expectEqual(@as(u64, 0xfeed_0000_0000_0001), view.events[0].source);
    try testing.expectEqual(@as(u32, 77), view.events[0].payload.unknown);
    try testing.expectEqualStrings("future", view.events[0].message);
    try testing.expectEqual(maplibre.RuntimeEventType.map_style_loaded, view.events[1].type);
    try testing.expect(view.events[1].payload == .empty);
    try testing.expectEqualStrings("loaded", view.events[1].message);
}

// An enum value the binding does not name crosses unchanged, so native is the
// one that rejects it, and a 64-bit identifier keeps its high bits both ways.
// Zig integer types mirror the C widths, so a narrowing would not compile.
test "open enums keep unknown values and 64-bit carriers round-trip" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    try fixture.loadStyle();

    const rejected = try support.resolve(try maplibre.mapSetStyleLayerVisibility(fixture.map, "background", @enumFromInt(900), null));
    try testing.expectEqual(maplibre.CommandDisposition.failed, rejected.disposition);
    try testing.expectError(error.InvalidArgument, rejected.statusError());

    // A jump supersedes the ease, which reports its identifier as it ends.
    const transition_id: u64 = 0x8000_0000_0000_0029;
    try support.expectCommitted(try maplibre.mapUpdateCamera(testing.allocator, fixture.map, .{
        .mode = .ease,
        .camera = .{ .zoom = 4.0 },
        .animation = .{ .duration_ms = 60_000, .transition_id = transition_id },
    }, null));
    try support.expectCommitted(try maplibre.mapUpdateCamera(testing.allocator, fixture.map, .{ .mode = .jump, .camera = .{ .zoom = 8.0 } }, null));
    var finished = try fixture.waitForEvent(.map_camera_transition_finished);
    defer finished.deinit();
    try testing.expectEqual(transition_id, finished.value.payload.camera_transition_finished.transition_id);
}

// The binding lowers an array input into storage native copies during the
// call, so the caller may reuse its buffer as soon as the submission returns.
test "an array input is copied at submission" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    try fixture.loadStyle();

    var pixels: [2 * 2 * 4]u8 = @splat(0x80);
    const submitted = try maplibre.mapSetStyleImage(testing.allocator, fixture.map, "copied", .{
        .width = 2,
        .height = 2,
        .stride = 2 * 4,
        .pixels = &pixels,
    }, null, null);
    @memset(&pixels, 0);
    try support.expectCommitted(submitted);

    var copy = (try support.resolve(try maplibre.mapGetStyleImage(testing.allocator, fixture.map, "copied", null))).?;
    defer copy.deinit();
    try testing.expectEqual(@as(usize, pixels.len), copy.value.pixels.len);
    for (copy.value.pixels) |byte| try testing.expectEqual(@as(u8, 0x80), byte);
}

// A struct's field defaults come from the header's field annotations, so the
// record they build matches what the native default function returns.
test "a record built from its field defaults equals the native default" {
    try testing.expectEqual(try maplibre.mapOptionsDefault(), maplibre.MapOptions{});
}
