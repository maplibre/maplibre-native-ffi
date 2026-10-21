//! The generated value shapes: strings, optional fields and masks, strided
//! batches and tagged unions, open enums and 64-bit carriers, and array inputs.

const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("fixture.zig");

// Host strings cross as views. Native rejects NUL in paths before a C-string
// consumer reads them, while other views preserve NUL.
test "host strings use views and native-lent strings decode" {
    const RawRequest = @typeInfo(@TypeOf(maplibre.ResourceRequestInfo.fromNative)).@"fn".param_types[1].?;
    var raw_request = std.mem.zeroes(RawRequest);
    raw_request.resolved_url = "custom://native-lent/é";
    var arena = std.heap.ArenaAllocator.init(testing.allocator);
    defer arena.deinit();
    const request = try maplibre.ResourceRequestInfo.fromNative(arena.allocator(), raw_request);
    try testing.expectEqualStrings("custom://native-lent/é", request.resolved_url.?);

    var options = try maplibre.runtimeOptionsDefault(testing.allocator);
    defer options.deinit();
    options.value.asset_path = "asset\x00path";
    var diagnostic: maplibre.Diagnostic = .{};
    try testing.expectError(error.InvalidArgument, maplibre.runtimeCreate(testing.allocator, options.value, &diagnostic));
    try testing.expect(diagnostic.raw_status != null);
    try testing.expect(std.mem.indexOf(u8, diagnostic.message(), "embedded NUL") != null);

    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
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

    var update = try maplibre.cameraUpdateDefault(testing.allocator);
    defer update.deinit();
    update.value.mode = .jump;
    update.value.camera = .{ .zoom = 3.0, .pitch = 30.0 };
    try support.expectCommitted(try maplibre.mapUpdateCamera(testing.allocator, fixture.map, update.value, null));
    update.value.camera = .{ .zoom = 5.0 };
    try support.expectCommitted(try maplibre.mapUpdateCamera(testing.allocator, fixture.map, update.value, null));
    const snapshot = try maplibre.mapGetSnapshot(fixture.map, null);
    try testing.expectEqual(@as(?f64, 5.0), snapshot.view.camera.zoom);
    try testing.expectApproxEqAbs(@as(f64, 30.0), snapshot.view.camera.pitch.?, 1e-9);

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
    const RawBatch = @typeInfo(@TypeOf(maplibre.EventBatchView.fromNative)).@"fn".param_types[1].?;
    const RawEvent = @typeInfo(@FieldType(RawBatch, "events")).pointer.child;
    const Record = extern struct { event: RawEvent, newer_fields: [24]u8 };

    const messages = "future\x00loaded\x00";
    var records = std.mem.zeroes([2]Record);
    records[0].event.type = 999;
    records[0].event.source = 0xfeed_0000_0000_0001;
    records[0].event.generation = 0x1_0000_0002;
    records[0].event.payload_type = 77;
    records[0].event.message = .{ .data = messages.ptr, .size = 6 };
    records[0].newer_fields = @splat(0xa5);
    records[1].event.type = @backingInt(maplibre.RuntimeEventType.map_style_loaded);
    records[1].event.message = .{ .data = messages.ptr + 7, .size = 6 };
    var raw = std.mem.zeroes(RawBatch);
    raw.size = @sizeOf(RawBatch);
    raw.event_size = @sizeOf(Record);
    raw.events = &records[0].event;
    raw.event_count = records.len;

    var arena = std.heap.ArenaAllocator.init(testing.allocator);
    defer arena.deinit();
    const view = try maplibre.EventBatchView.fromNative(arena.allocator(), raw);
    try testing.expectEqual(@as(usize, 2), view.events.len);
    try testing.expectEqual(@as(u32, 999), @backingInt(view.events[0].type));
    try testing.expectEqual(@as(u64, 0xfeed_0000_0000_0001), view.events[0].source);
    try testing.expectEqual(@as(u64, 0x1_0000_0002), view.events[0].generation);
    try testing.expectEqual(@as(u32, 77), view.events[0].payload.unknown);
    try testing.expectEqualStrings("future", view.events[0].message);
    try testing.expectEqual(maplibre.RuntimeEventType.map_style_loaded, view.events[1].type);
    try testing.expect(view.events[1].payload == .empty);
    try testing.expectEqualStrings("loaded", view.events[1].message);
}

// An enum value the binding does not name crosses unchanged, so native is the
// one that rejects it, and a 64-bit identifier keeps its high bits.
// Zig integer types mirror the C widths, so a narrowing would not compile.
test "open enums keep unknown values and 64-bit carriers round-trip" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    try fixture.loadStyle();

    try testing.expectError(error.InvalidArgument, maplibre.networkSetStatus(@fromBackingInt(900), null));

    // Cancelling the identifier ends only the ease that carries it, so the
    // identifier crossed whole both times. The end handler sits two records
    // deep in the command's input and runs once, before the cancellation
    // completes.
    const transition_id: u64 = 0x8000_0000_0000_0029;
    var ended = support.EndProbe{};
    var update = try maplibre.cameraUpdateDefault(testing.allocator);
    defer update.deinit();
    update.value.mode = .ease;
    update.value.camera = .{ .zoom = 4.0 };
    update.value.animation = .{ .duration_ms = 60_000, .transition_id = transition_id, .end_handler = ended.handler() };
    try support.expectCommitted(try maplibre.mapUpdateCamera(testing.allocator, fixture.map, update.value, null));
    try support.expectCommitted(try maplibre.mapCancelCameraTransition(fixture.map, transition_id & 0x7fff_ffff_ffff_ffff, null));
    try testing.expectEqual(@as(u32, 0), ended.ends.load(.acquire));
    try support.expectCommitted(try maplibre.mapCancelCameraTransition(fixture.map, transition_id, null));
    try ended.releases.waitFor(1);
    try testing.expectEqual(@as(u32, 1), ended.ends.load(.acquire));
    try testing.expectEqual(maplibre.CameraTransitionOutcome.cancelled, ended.outcome);
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

test "native record defaults round trip through generated conversion" {
    const options = try maplibre.mapOptionsDefault();
    try testing.expectEqual(options, maplibre.MapOptions.fromNative(options.toNative()));
}
