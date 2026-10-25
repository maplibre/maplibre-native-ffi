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
    const lent_url = "custom://native-lent/é";
    raw_request.resolved_url = .{ .data = lent_url, .size = lent_url.len };
    var arena = std.heap.ArenaAllocator.init(testing.allocator);
    defer arena.deinit();
    const request = try maplibre.ResourceRequestInfo.fromNative(arena.allocator(), raw_request);
    try testing.expectEqualStrings("custom://native-lent/é", request.resolved_url);

    var options = try maplibre.runtimeOptionsDefault(testing.allocator);
    defer options.deinit();
    options.value.asset_path = "asset\x00path";
    var diagnostic: maplibre.Diagnostic = .{};
    try testing.expectError(error.InvalidArgument, maplibre.runtimeCreate(options.value, &diagnostic));
    try testing.expect(diagnostic.status != null);
    try testing.expect(std.mem.indexOf(u8, diagnostic.message(), "embedded NUL") != null);

    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    try fixture.loadStyle();
    const source_id = "nul\x00source";
    try support.expectCommitted(try fixture.map.addStyleSourceJson(source_id, "{\"type\":\"geojson\",\"data\":{\"type\":\"FeatureCollection\",\"features\":[]}}", null));
    var info = (try support.resolve(try fixture.map.getStyleSource(testing.allocator, source_id, null))).?;
    defer info.deinit();
    try testing.expectEqual(maplibre.StyleSourceType.geojson, info.value.type);
    try testing.expect(try support.resolve(try fixture.map.getStyleSource(testing.allocator, "nul", null)) == null);
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
    try support.expectCommitted(try fixture.map.updateCamera(update.value, null));
    update.value.camera = .{ .zoom = 5.0 };
    try support.expectCommitted(try fixture.map.updateCamera(update.value, null));
    const snapshot = try fixture.map.getSnapshot(null);
    try testing.expectEqual(@as(?f64, 5.0), snapshot.view.camera.zoom);
    try testing.expectApproxEqAbs(@as(f64, 30.0), snapshot.view.camera.pitch.?, 1e-9);

    var mask = maplibre.RuntimeEventFlag.all;
    mask.map_tile_action = false;
    mask.offline_region_status_changed = false;
    try fixture.runtime.setEventMask(mask, null);
    try testing.expectEqual(mask, try fixture.runtime.getEventMask(null));
}

// Native steps through a batch by the stride it reports, which may exceed the
// record size the binding was built with, and a payload tag the binding does
// not know decodes as `.unknown` with its raw value. The event handler's
// trampoline copies the batch for the host callback.
test "a strided event batch with an unknown payload arm decodes without loss" {
    const Roots = @typeInfo(@typeInfo(@TypeOf(maplibre.RuntimeEventHandler.toNative)).@"fn".param_types[1].?).pointer.child;
    const RawHandler = @typeInfo(@TypeOf(maplibre.RuntimeEventHandler.toNative)).@"fn".return_type.?;
    const Callback = @typeInfo(@typeInfo(@FieldType(@typeInfo(RawHandler).error_union.payload, "callback")).optional.child).pointer.child;
    const RawBatch = @typeInfo(@typeInfo(Callback).@"fn".param_types[1].?).pointer.child;
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

    // The trampoline reports a callback error rather than returning it, so the
    // probe keeps the outcome of its checks.
    const Probe = struct {
        checked: anyerror!void = error.NotDelivered,
        fn receive(self: *@This(), events: []const maplibre.RuntimeEvent) anyerror!void {
            self.checked = expectDecoded(events);
        }
        fn expectDecoded(events: []const maplibre.RuntimeEvent) !void {
            try testing.expectEqual(@as(usize, 2), events.len);
            try testing.expectEqual(@as(u32, 999), @backingInt(events[0].type));
            try testing.expectEqual(@as(u64, 0xfeed_0000_0000_0001), events[0].source);
            try testing.expectEqual(@as(u64, 0x1_0000_0002), events[0].generation);
            try testing.expectEqual(@as(u32, 77), events[0].payload.unknown);
            try testing.expectEqualStrings("future", events[0].message);
            try testing.expectEqual(maplibre.RuntimeEventType.map_style_loaded, events[1].type);
            try testing.expect(events[1].payload == .empty);
            try testing.expectEqualStrings("loaded", events[1].message);
        }
    };
    var probe = Probe{};
    var roots: Roots = .{};
    defer roots.deinit();
    const handler = try maplibre.RuntimeEventHandler.init(&probe, Probe.receive, null).toNative(&roots);
    handler.callback.?(handler.user_data, &raw);
    try probe.checked;
}

// A record of only a tag and its union is that union: the conversion writes the
// size and the variant's tag. Only a union that native returns keeps a tag the
// binding does not know, and that tag cannot be submitted.
test "a tag-and-union record converts as the union itself" {
    var arena = std.heap.ArenaAllocator.init(testing.allocator);
    defer arena.deinit();
    const geometry: maplibre.RenderedQueryGeometry = .{ .box = .{ .min = .{ .x = 1, .y = 2 }, .max = .{ .x = 3, .y = 4 } } };
    const raw_geometry = try geometry.toNative(arena.allocator());
    try testing.expectEqual(@sizeOf(@TypeOf(raw_geometry)), raw_geometry.size);
    try testing.expectEqual(geometry, try maplibre.RenderedQueryGeometry.fromNative(arena.allocator(), raw_geometry));

    const RawDefinition = @typeInfo(@TypeOf(maplibre.OfflineRegionDefinition.fromNative)).@"fn".param_types[1].?;
    var raw_definition = std.mem.zeroes(RawDefinition);
    raw_definition.type = 77;
    const definition = try maplibre.OfflineRegionDefinition.fromNative(arena.allocator(), raw_definition);
    try testing.expectEqual(@as(u32, 77), definition.unknown);
    try testing.expectError(error.InvalidArgument, definition.toNative());
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
    try support.expectCommitted(try fixture.map.updateCamera(update.value, null));
    try support.expectCommitted(try fixture.map.cancelCameraTransition(transition_id & 0x7fff_ffff_ffff_ffff, null));
    try testing.expectEqual(@as(u32, 0), ended.ends.load(.acquire));
    try support.expectCommitted(try fixture.map.cancelCameraTransition(transition_id, null));
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
    const submitted = try fixture.map.setStyleImage(testing.allocator, "copied", .{
        .width = 2,
        .height = 2,
        .stride = 2 * 4,
        .pixels = &pixels,
    }, null, null);
    @memset(&pixels, 0);
    try support.expectCommitted(submitted);

    var copy = (try support.resolve(try fixture.map.getStyleImage(testing.allocator, "copied", null))).?;
    defer copy.deinit();
    try testing.expectEqual(@as(usize, pixels.len), copy.value.pixels.len);
    for (copy.value.pixels) |byte| try testing.expectEqual(@as(u8, 0x80), byte);
}

test "native record defaults round trip through generated conversion" {
    const options = maplibre.mapOptionsDefault();
    try testing.expectEqual(options, maplibre.MapOptions.fromNative(options.toNative()));
}

test "synchronous calls fill caller-sized outputs in input order" {
    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    var projection = try maplibre.mapProjectionCreate((try fixture.map.getSnapshot(null)).view, null);
    defer projection.release();
    const coordinates = [_]maplibre.LatLng{ .{ .latitude = 0, .longitude = 0 }, .{ .latitude = 10, .longitude = 20 }, .{ .latitude = -12, .longitude = -30 } };
    var points = try projection.pixelsForLatLngs(testing.allocator, &coordinates, null);
    defer points.deinit();
    try testing.expectEqual(coordinates.len, points.value.len);
    var wrapped = try projection.latLngsForPixels(testing.allocator, points.value, null);
    defer wrapped.deinit();
    var unwrapped = try projection.latLngsForPixelsUnwrapped(testing.allocator, points.value, null);
    defer unwrapped.deinit();
    for (coordinates, points.value, wrapped.value, unwrapped.value) |coordinate, point, single_wrapped, single_unwrapped| {
        try testing.expectEqualDeep(try projection.pixelForLatLng(coordinate, null), point);
        try testing.expectEqualDeep(try projection.latLngForPixel(point, null), single_wrapped);
        try testing.expectEqualDeep(try projection.latLngForPixelUnwrapped(point, null), single_unwrapped);
    }
    var empty_points = try projection.pixelsForLatLngs(testing.allocator, &.{}, null);
    defer empty_points.deinit();
    try testing.expectEqual(@as(usize, 0), empty_points.value.len);
    var empty_wrapped = try projection.latLngsForPixels(testing.allocator, &.{}, null);
    defer empty_wrapped.deinit();
    try testing.expectEqual(@as(usize, 0), empty_wrapped.value.len);
    var empty_unwrapped = try projection.latLngsForPixelsUnwrapped(testing.allocator, &.{}, null);
    defer empty_unwrapped.deinit();
    try testing.expectEqual(@as(usize, 0), empty_unwrapped.value.len);
}

test "caller-sized text returns results longer than its first storage" {
    var zoom = try maplibre.styleExpressionCreate("[\"interpolate\", [\"linear\"], [\"zoom\"], 0, 0, 10, 100]", null, null);
    defer zoom.release();
    var level = try zoom.evaluate(testing.allocator, .{ .zoom = 5, .available_images = &.{} }, null);
    defer level.deinit();
    try testing.expectEqualStrings("50.0", level.value);

    // The result outgrows the call's first storage, so the call runs again.
    var text: [1002]u8 = undefined;
    @memset(&text, 'a');
    text[0] = '"';
    text[text.len - 1] = '"';
    var long = try maplibre.styleExpressionCreate(&text, null, null);
    defer long.release();
    var result = try long.evaluate(testing.allocator, null, null);
    defer result.deinit();
    try testing.expectEqualStrings(&text, result.value);
}
