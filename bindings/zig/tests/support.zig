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

/// Closes a runtime and waits for its native teardown, so a test leaves no
/// native thread or resource behind.
pub fn closeRuntime(runtime: *maplibre.Runtime) !void {
    var teardown = try maplibre.runtimeRelease(handle(runtime));
    defer teardown.deinit();
    try teardown.wait(null);
}

/// Closes a map and waits for its native teardown.
pub fn closeMap(map: *maplibre.Map) !void {
    var teardown = try maplibre.mapRelease(handle(map));
    defer teardown.deinit();
    try teardown.wait(null);
}

pub fn sleepOneMillisecond() !void {
    try testing.io.sleep(.fromMilliseconds(1), .awake);
}

pub fn handle(value: anytype) if (@typeInfo(@TypeOf(value)) == .pointer) @typeInfo(@TypeOf(value)).pointer.child else @TypeOf(value) {
    return if (@typeInfo(@TypeOf(value)) == .pointer) value.* else value;
}

pub fn createRuntime(options: anytype) !maplibre.Runtime {
    return createRuntimeWithDiagnostics(options, null);
}

/// Creates a runtime whose handles report failures into `diagnostic_store`.
pub fn createRuntimeWithDiagnostics(options: anytype, diagnostic_store: ?*maplibre.DiagnosticStore) !maplibre.Runtime {
    var defaults = try maplibre.runtimeOptionsDefault(testing.allocator);
    defer defaults.deinit();
    inline for (@typeInfo(@TypeOf(options)).@"struct".fields) |field| @field(defaults.value, field.name) = @field(options, field.name);
    return maplibre.runtimeCreate(testing.allocator, defaults.value, diagnostic_store);
}

pub fn waitForEvent(runtime: *maplibre.Runtime, event_type: maplibre.RuntimeEventType) !bool {
    for (0..1000) |_| {
        var batch = try maplibre.runtimeDrainEvents(runtime.*);
        defer batch.deinit();
        var copy = try maplibre.eventBatchGet(testing.allocator, batch);
        defer copy.deinit();
        for (copy.value.events) |event| if (event.type == event_type) {
            return true;
        };
        try sleepOneMillisecond();
    }
    return false;
}

pub fn waitForOwnedEvent(runtime: *maplibre.Runtime, event_type: maplibre.RuntimeEventType) !maplibre.OwnedValue(maplibre.RuntimeEvent) {
    for (0..5000) |_| {
        var batch = try maplibre.runtimeDrainEvents(runtime.*);
        defer batch.deinit();
        var copy = try maplibre.eventBatchGet(testing.allocator, batch);
        for (copy.value.events) |event| if (event.type == event_type) {
            return .{ .arena = copy.arena, .value = event };
        };
        copy.deinit();
        try sleepOneMillisecond();
    }
    return error.EventNotObserved;
}

/// Waits for a future's terminal value and releases the future.
pub fn resolve(comptime T: type, future_value: maplibre.Future(T)) !T {
    var future = future_value;
    defer future.deinit();
    return future.wait(null);
}

/// Waits for an accepted command and asserts that it committed.
pub fn expectCommitted(future_value: anytype) !void {
    var future = future_value;
    defer future.deinit();
    const result = try future.wait(null);
    if (@TypeOf(result) != void) try testing.expectEqual(maplibre.CommandDisposition.committed, result.disposition);
}

/// Verifies that an accepted command completed with a native failure.
pub fn expectCommandError(
    future_value: maplibre.Future(maplibre.CommandCompletion),
    expected: anyerror,
) !void {
    var future = future_value;
    defer future.deinit();
    const completion = try future.wait(null);
    try testing.expectEqual(maplibre.CommandDisposition.failed, completion.disposition);
    try testing.expectError(expected, completion.statusError());
    try testing.expect((try future.diagnostic()).len != 0);
}

/// Awaits `future`'s commit and returns a map snapshot that observes it: the
/// completion reports the published generation, and a snapshot at or past that
/// generation carries the committed state.
pub fn snapshotAfterCommand(
    map: *maplibre.Map,
    future: maplibre.Future(maplibre.CommandCompletion),
) !maplibre.MapSnapshot {
    const finished = try resolve(maplibre.CommandCompletion, future);
    try testing.expectEqual(maplibre.CommandDisposition.committed, finished.disposition);
    try testing.expect(finished.generation != 0);
    const snapshot = try maplibre.mapSnapshotGet(handle(map));
    try testing.expect(snapshot.generation >= finished.generation);
    return snapshot;
}

/// Drains every queued event, reporting how many the batch carried.
pub fn drainEvents(runtime: *maplibre.Runtime) !usize {
    var batch = try maplibre.runtimeDrainEvents(handle(runtime));
    defer batch.deinit();
    var copy = try maplibre.eventBatchGet(testing.allocator, batch);
    defer copy.deinit();
    return copy.value.events.len;
}

/// Creates a map with `style_json` loaded.
pub fn createLoadedMap(runtime: *maplibre.Runtime) !maplibre.Map {
    var map = try createMap(runtime, .{});
    errdefer closeMap(&map) catch {};
    try expectCommitted(try maplibre.mapSetStyleJson(handle(map), style_json));
    try testing.expect(try waitForEvent(runtime, .map_style_loaded));
    return map;
}

pub fn createMap(runtime: *maplibre.Runtime, options: anytype) !maplibre.Map {
    var defaults = try maplibre.mapOptionsDefault();
    inline for (@typeInfo(@TypeOf(options)).@"struct".fields) |field| {
        if (comptime std.mem.eql(u8, field.name, "width") or std.mem.eql(u8, field.name, "height") or std.mem.eql(u8, field.name, "scale_factor")) {
            @field(defaults.initial_extent, field.name) = @field(options, field.name);
        } else if (comptime std.mem.eql(u8, field.name, "mode")) {
            defaults.map_mode = @field(options, field.name);
        } else @field(defaults, field.name) = @field(options, field.name);
    }
    return resolve(maplibre.Map, try maplibre.mapCreate(testing.allocator, runtime.*, defaults));
}

pub fn waitForBarrier(runtime: *maplibre.Runtime) !void {
    _ = try resolve(void, try maplibre.runtimeBarrier(handle(runtime)));
}

/// Copies one source's metadata, reporting null when no source has the ID.
pub fn styleSourceInfo(map: *maplibre.Map, source_id: []const u8) !?maplibre.generated.OwnedValue(maplibre.generated.StyleSourceResult) {
    return resolve(?maplibre.generated.OwnedValue(maplibre.generated.StyleSourceResult), try maplibre.mapGetStyleSourceInfo(testing.allocator, handle(map), source_id));
}

/// Existence via the info getter's found flag.
pub fn styleSourceExists(map: *maplibre.Map, source_id: []const u8) !bool {
    var info = (try styleSourceInfo(map, source_id)) orelse return false;
    info.deinit();
    return true;
}

pub fn styleSourceType(map: *maplibre.Map, source_id: []const u8) !?maplibre.StyleSourceType {
    var info = (try styleSourceInfo(map, source_id)) orelse return null;
    defer info.deinit();
    return info.value.info.type;
}

/// Waits for a removal command: true when it commits, false when the ID names
/// nothing, and the command's reported failure otherwise.
fn awaitRemoval(future: maplibre.Future(maplibre.CommandCompletion)) !bool {
    const completion_result = try resolve(maplibre.CommandCompletion, future);
    return switch (completion_result.disposition) {
        .committed => true,
        .failed => {
            completion_result.statusError() catch |err| {
                if (err == error.NotFound) return false;
                return err;
            };
            return error.UnexpectedCommandDisposition;
        },
        else => error.UnexpectedCommandDisposition,
    };
}

pub fn removeStyleSource(map: *maplibre.Map, source_id: []const u8) !bool {
    return awaitRemoval(try maplibre.mapRemoveStyleSource(handle(map), source_id));
}

pub fn removeStyleLayer(map: *maplibre.Map, layer_id: []const u8) !bool {
    return awaitRemoval(try maplibre.mapRemoveStyleLayer(handle(map), layer_id));
}

pub fn removeStyleImage(map: *maplibre.Map, image_id: []const u8) !bool {
    return awaitRemoval(try maplibre.mapRemoveStyleImage(handle(map), image_id));
}

/// Copies fixed layer metadata, reporting null when no layer has the ID.
pub fn styleLayerInfo(map: *maplibre.Map, layer_id: []const u8) !?maplibre.generated.OwnedValue(maplibre.generated.StyleLayerResult) {
    return resolve(?maplibre.generated.OwnedValue(maplibre.generated.StyleLayerResult), try maplibre.mapGetStyleLayerInfo(testing.allocator, handle(map), layer_id));
}

/// Existence via the info getter's found flag.
pub fn styleLayerExists(map: *maplibre.Map, layer_id: []const u8) !bool {
    var info = (try styleLayerInfo(map, layer_id)) orelse return false;
    info.deinit();
    return true;
}

pub fn loadedStyleJson(map: *maplibre.Map) !maplibre.OwnedValue([]const u8) {
    return resolve(maplibre.OwnedValue([]const u8), try maplibre.mapLoadedStyleJson(testing.allocator, handle(map)));
}

pub fn styleUrl(map: *maplibre.Map) !maplibre.OwnedValue([]const u8) {
    return resolve(maplibre.OwnedValue([]const u8), try maplibre.mapStyleUrl(testing.allocator, handle(map)));
}

pub fn listStyleSourceIds(map: *maplibre.Map) !maplibre.OwnedValue([]const []const u8) {
    return resolve(maplibre.OwnedValue([]const []const u8), try maplibre.mapListStyleSourceIds(testing.allocator, handle(map)));
}

pub fn listStyleLayerIds(map: *maplibre.Map) !maplibre.OwnedValue([]const []const u8) {
    return resolve(maplibre.OwnedValue([]const []const u8), try maplibre.mapListStyleLayerIds(testing.allocator, handle(map)));
}

pub fn styleSourceAttribution(map: *maplibre.Map, source_id: []const u8) !?maplibre.OwnedValue([]const u8) {
    return resolve(?maplibre.OwnedValue([]const u8), try maplibre.mapCopyStyleSourceAttribution(testing.allocator, handle(map), source_id));
}

pub fn styleSourceUrl(map: *maplibre.Map, source_id: []const u8) !?maplibre.OwnedValue([]const u8) {
    return resolve(?maplibre.OwnedValue([]const u8), try maplibre.mapCopyStyleSourceUrl(testing.allocator, handle(map), source_id));
}

pub fn styleSourceTileUrls(map: *maplibre.Map, source_id: []const u8) !?maplibre.generated.OwnedValue(maplibre.generated.StyleSourceTileUrlsResult) {
    return resolve(?maplibre.generated.OwnedValue(maplibre.generated.StyleSourceTileUrlsResult), try maplibre.mapGetStyleSourceTileUrls(testing.allocator, handle(map), source_id));
}

pub fn expectStyleLayerType(map: *maplibre.Map, layer_id: []const u8, expected: []const u8) !void {
    var info = (try styleLayerInfo(map, layer_id)) orelse return error.NotFound;
    defer info.deinit();
    try testing.expectEqualStrings(expected, info.value.info.type);
}

pub fn styleLayerJson(map: *maplibre.Map, layer_id: []const u8) !?maplibre.OwnedValue([]const u8) {
    return resolve(?maplibre.OwnedValue([]const u8), try maplibre.mapGetStyleLayerJson(testing.allocator, handle(map), layer_id));
}

pub fn styleImageInfo(map: *maplibre.Map, image_id: []const u8) !?maplibre.generated.OwnedValue(maplibre.generated.StyleImageResult) {
    return resolve(?maplibre.generated.OwnedValue(maplibre.generated.StyleImageResult), try maplibre.mapGetStyleImageInfo(testing.allocator, handle(map), image_id));
}

pub fn styleImageStretches(map: *maplibre.Map, image_id: []const u8) !?maplibre.generated.OwnedValue(maplibre.generated.StyleImageStretchesResult) {
    return resolve(?maplibre.generated.OwnedValue(maplibre.generated.StyleImageStretchesResult), try maplibre.mapCopyStyleImageStretches(testing.allocator, handle(map), image_id));
}

pub fn imageSourceCoordinates(map: *maplibre.Map, source_id: []const u8) !?[4]maplibre.LatLng {
    var snapshot = (try resolve(?maplibre.generated.OwnedValue([]const maplibre.LatLng), try maplibre.mapGetImageSourceCoordinates(testing.allocator, handle(map), source_id))) orelse return null;
    defer snapshot.deinit();
    if (snapshot.value.len != 4) return error.NativeError;
    return snapshot.value[0..4].*;
}

pub fn layerSourceLayer(map: *maplibre.Map, layer_id: []const u8) !?maplibre.OwnedValue([]const u8) {
    return resolve(?maplibre.OwnedValue([]const u8), try maplibre.mapCopyLayerSourceLayer(testing.allocator, handle(map), layer_id));
}

pub fn layerSourceId(map: *maplibre.Map, layer_id: []const u8) !?maplibre.OwnedValue([]const u8) {
    return resolve(?maplibre.OwnedValue([]const u8), try maplibre.mapCopyLayerSourceId(testing.allocator, handle(map), layer_id));
}

pub fn layerProperty(map: *maplibre.Map, layer_id: []const u8, property_name: []const u8) !?maplibre.OwnedValue([]const u8) {
    return resolve(?maplibre.OwnedValue([]const u8), try maplibre.mapGetLayerProperty(testing.allocator, handle(map), layer_id, property_name));
}

pub fn layerFilter(map: *maplibre.Map, layer_id: []const u8) !?maplibre.OwnedValue([]const u8) {
    return resolve(?maplibre.OwnedValue([]const u8), try maplibre.mapGetLayerFilter(testing.allocator, handle(map), layer_id));
}

pub fn styleLightProperty(map: *maplibre.Map, property_name: []const u8) !?maplibre.OwnedValue([]const u8) {
    return resolve(?maplibre.OwnedValue([]const u8), try maplibre.mapGetStyleLightProperty(testing.allocator, handle(map), property_name));
}

/// Existence via the info getter's found flag.
pub fn styleImageExists(map: *maplibre.Map, image_id: []const u8) !bool {
    var info = (try styleImageInfo(map, image_id)) orelse return false;
    defer info.deinit();
    return true;
}

pub fn styleImagePixels(map: *maplibre.Map, image_id: []const u8) !?maplibre.OwnedValue([]const u8) {
    return resolve(?maplibre.OwnedValue([]const u8), try maplibre.mapCopyStyleImagePremultipliedRgba8(testing.allocator, handle(map), image_id));
}

pub fn styleTransitionOptions(map: *maplibre.Map) !maplibre.StyleTransitionOptions {
    return resolve(maplibre.StyleTransitionOptions, try maplibre.mapGetStyleTransitionOptions(handle(map)));
}

pub fn listIndexOf(list: maplibre.OwnedValue([]const []const u8), value: []const u8) ?usize {
    for (list.value, 0..) |item, index| {
        if (std.mem.eql(u8, item, value)) return index;
    }
    return null;
}

pub fn expectListContains(list: maplibre.OwnedValue([]const []const u8), value: []const u8) !void {
    try testing.expect(listIndexOf(list, value) != null);
}

// Turns a driver poll waits before giving up. The first `spin_turns` yield, so
// work that lands immediately costs nothing; the rest sleep a millisecond
// each, which puts a wall-clock bound on the wait. Yields alone cannot: they
// measure scheduler turns, and on an idle host a hundred thousand of them
// elapse in tens of milliseconds -- less than a real frame takes, so the wait
// expired while the frame was still on its way.
pub const spin_turns = 1_000;
pub const wait_turns = spin_turns + 30_000;

pub fn waitOneTurn(turn: usize) !void {
    if (turn < spin_turns) return std.Thread.yield();
    try sleepOneMillisecond();
}

/// Waits for a render-session future, servicing the driver between polls when
/// the session's driver is the calling thread.
pub fn resolveSessionFuture(
    comptime T: type,
    session: maplibre.RenderSession,
    future_value: maplibre.Future(T),
    service_driver: bool,
) !T {
    var future = future_value;
    defer future.deinit();
    for (0..wait_turns) |turn| {
        if (try future.poll()) return future.wait(null);
        if (service_driver) _ = maplibre.renderSessionServiceDriverWork(handle(session), 64) catch 0;
        try waitOneTurn(turn);
    }
    return error.OperationTimedOut;
}

pub fn finishOperation(
    session: maplibre.RenderSession,
    future: anytype,
    service_driver: bool,
) !void {
    const T = @TypeOf(future).Value;
    const result = try resolveSessionFuture(T, session, future, service_driver);
    if (T == maplibre.CommandCompletion) try result.statusError();
}

pub fn finishAttachment(
    attachment: anytype,
    service_driver: bool,
) !maplibre.RenderSession {
    errdefer {
        const session = attachment.session;
        maplibre.renderSessionDestroy(handle(session)) catch {};
    }
    try finishOperation(attachment.session, attachment.ready, service_driver);
    return attachment.session;
}

pub fn closeSession(session: *maplibre.RenderSession, service_driver: bool) !void {
    try finishOperation(session.*, try maplibre.renderSessionDetach(handle(session)), service_driver);
    try maplibre.renderSessionDestroy(handle(session));
}

var next_frame_token: std.atomic.Value(u32) = .init(1);

pub fn nextFrameToken() u64 {
    return next_frame_token.fetchAdd(1, .seq_cst);
}

/// Requests one frame carrying `demand` and returns the result for its token.
pub fn renderFrameWithDemand(
    session: maplibre.RenderSession,
    demand: maplibre.FrameDemand,
    service_driver: bool,
) !maplibre.RenderFrameResult {
    try maplibre.renderSessionRequestFrame(testing.allocator, handle(session), demand);
    for (0..wait_turns) |turn| {
        if (service_driver) _ = maplibre.renderSessionServiceDriverWork(handle(session), 64) catch 0;
        var batch = maplibre.renderSessionDrainFrameResults(handle(session)) catch |err| {
            if (err == error.NotReady) {
                try waitOneTurn(turn);
                continue;
            }
            return err;
        };
        defer batch.deinit();
        for (0..try maplibre.renderFrameBatchCount(batch)) |index| {
            const result = try maplibre.renderFrameBatchGet(batch, index);
            if (result.token == demand.token) return result;
        }
        try waitOneTurn(turn);
    }
    return error.FrameTimedOut;
}

/// Requests one presenting frame and returns its result.
pub fn renderFrame(
    session: maplibre.RenderSession,
    if_needed: bool,
    service_driver: bool,
) !maplibre.RenderFrameResult {
    const capabilities = try maplibre.renderSessionGetCapabilities(handle(session));
    return renderFrameWithDemand(session, .{
        .flags = .{ .if_needed = if_needed, .present = capabilities.flags.presentation },
        .token = nextFrameToken(),
    }, service_driver);
}

/// Renders until one frame reports `.rendered`, tolerating the pending
/// dispositions a resize or a target swap produces.
pub fn expectRenderedFrame(
    session: maplibre.RenderSession,
    service_driver: bool,
) !maplibre.RenderFrameResult {
    for (0..1_000) |_| {
        const result = try renderFrame(session, false, service_driver);
        switch (result.disposition) {
            .rendered => return result,
            .size_pending, .target_not_ready => {},
            else => return error.UnexpectedFrameDisposition,
        }
    }
    return error.FrameDidNotRender;
}

pub fn drainEventSnapshot(runtime: anytype) !maplibre.OwnedValue(maplibre.RuntimeEventBatchView) {
    var batch = try maplibre.runtimeDrainEvents(handle(runtime));
    defer batch.deinit();
    return maplibre.eventBatchGet(testing.allocator, batch);
}
