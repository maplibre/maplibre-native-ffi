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
pub fn closeRuntime(runtime: *maplibre.RuntimeHandle) !void {
    var teardown = try runtime.close();
    defer teardown.deinit();
    try teardown.wait(null);
}

/// Closes a map and waits for its native teardown.
pub fn closeMap(map: *maplibre.MapHandle) !void {
    var teardown = try map.close();
    defer teardown.deinit();
    try teardown.wait(null);
}

pub fn sleepOneMillisecond() !void {
    try testing.io.sleep(.fromMilliseconds(1), .awake);
}

/// Waits for an autonomously produced event of `event_type`, reporting whether
/// it arrived.
pub fn waitForEvent(runtime: *maplibre.RuntimeHandle, event_type: maplibre.RuntimeEventType) !bool {
    for (0..1000) |_| {
        var batch = try runtime.drainEvents(testing.allocator);
        defer batch.deinit();
        for (0..batch.len()) |index| {
            const event = try batch.at(index);
            if (std.meta.eql(event.event_type, event_type)) return true;
        }
        try sleepOneMillisecond();
    }
    return false;
}

/// Waits for an event of `event_type` and copies it out of its batch. The caller
/// deinits the returned event.
pub fn waitForOwnedEvent(
    runtime: *maplibre.RuntimeHandle,
    event_type: maplibre.RuntimeEventType,
) !maplibre.RuntimeEvent {
    for (0..5000) |_| {
        var batch = try runtime.drainEvents(testing.allocator);
        defer batch.deinit();
        for (0..batch.len()) |index| {
            const event = try batch.at(index);
            if (std.meta.eql(event.event_type, event_type)) return try event.clone(testing.allocator);
        }
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
pub fn expectCommitted(future: maplibre.Future(maplibre.CommandCompletion)) !void {
    try testing.expectEqual(
        maplibre.CommandDisposition.committed,
        (try resolve(maplibre.CommandCompletion, future)).disposition,
    );
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
    map: *maplibre.MapHandle,
    future: maplibre.Future(maplibre.CommandCompletion),
) !maplibre.MapSnapshot {
    const finished = try resolve(maplibre.CommandCompletion, future);
    try testing.expectEqual(maplibre.CommandDisposition.committed, finished.disposition);
    try testing.expect(finished.generation != 0);
    const snapshot = try map.snapshot();
    try testing.expect(snapshot.generation >= finished.generation);
    return snapshot;
}

/// Drains every queued event, reporting how many the batch carried.
pub fn drainEvents(runtime: *maplibre.RuntimeHandle) !usize {
    var batch = try runtime.drainEvents(testing.allocator);
    defer batch.deinit();
    return batch.len();
}

/// Creates a map with `style_json` loaded.
pub fn createLoadedMap(runtime: *maplibre.RuntimeHandle) !maplibre.MapHandle {
    var map = try createMap(runtime, .{});
    errdefer closeMap(&map) catch {};
    try expectCommitted(try map.setStyleJson(style_json));
    try testing.expect(try waitForEvent(runtime, .map_style_loaded));
    return map;
}

pub fn createMap(runtime: *maplibre.RuntimeHandle, options: maplibre.MapOptions) !maplibre.MapHandle {
    var future = try maplibre.MapHandle.create(runtime, options);
    defer future.deinit();
    return future.wait(null);
}

pub fn waitForBarrier(runtime: *maplibre.RuntimeHandle) !void {
    _ = try resolve(void, try runtime.barrier());
}

/// Copies one source's metadata, reporting null when no source has the ID.
pub fn styleSourceInfo(map: *maplibre.MapHandle, source_id: []const u8) !?maplibre.StyleSourceInfo {
    return resolve(?maplibre.StyleSourceInfo, try map.getStyleSourceInfo(testing.allocator, source_id));
}

/// Existence via the info getter's found flag.
pub fn styleSourceExists(map: *maplibre.MapHandle, source_id: []const u8) !bool {
    var info = (try styleSourceInfo(map, source_id)) orelse return false;
    info.deinit();
    return true;
}

pub fn styleSourceType(map: *maplibre.MapHandle, source_id: []const u8) !?maplibre.StyleSourceType {
    var info = (try styleSourceInfo(map, source_id)) orelse return null;
    defer info.deinit();
    return info.source_type;
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

pub fn removeStyleSource(map: *maplibre.MapHandle, source_id: []const u8) !bool {
    return awaitRemoval(try map.removeStyleSource(testing.allocator, source_id));
}

pub fn removeStyleLayer(map: *maplibre.MapHandle, layer_id: []const u8) !bool {
    return awaitRemoval(try map.removeStyleLayer(testing.allocator, layer_id));
}

pub fn removeStyleImage(map: *maplibre.MapHandle, image_id: []const u8) !bool {
    return awaitRemoval(try map.removeStyleImage(testing.allocator, image_id));
}

/// Copies fixed layer metadata, reporting null when no layer has the ID.
pub fn styleLayerInfo(map: *maplibre.MapHandle, layer_id: []const u8) !?maplibre.StyleLayerInfo {
    return resolve(?maplibre.StyleLayerInfo, try map.getStyleLayerInfo(testing.allocator, layer_id));
}

/// Existence via the info getter's found flag.
pub fn styleLayerExists(map: *maplibre.MapHandle, layer_id: []const u8) !bool {
    var info = (try styleLayerInfo(map, layer_id)) orelse return false;
    info.deinit();
    return true;
}

pub fn loadedStyleJson(map: *maplibre.MapHandle) !maplibre.OwnedString {
    return resolve(maplibre.OwnedString, try map.loadedStyleJson(testing.allocator));
}

pub fn styleUrl(map: *maplibre.MapHandle) !maplibre.OwnedString {
    return resolve(maplibre.OwnedString, try map.styleUrl(testing.allocator));
}

pub fn listStyleSourceIds(map: *maplibre.MapHandle) !maplibre.StringList {
    return resolve(maplibre.StringList, try map.listStyleSourceIds(testing.allocator));
}

pub fn listStyleLayerIds(map: *maplibre.MapHandle) !maplibre.StringList {
    return resolve(maplibre.StringList, try map.listStyleLayerIds(testing.allocator));
}

pub fn styleSourceAttribution(map: *maplibre.MapHandle, source_id: []const u8) !?maplibre.OwnedString {
    return resolve(?maplibre.OwnedString, try map.copyStyleSourceAttribution(testing.allocator, source_id));
}

pub fn styleSourceUrl(map: *maplibre.MapHandle, source_id: []const u8) !?maplibre.OwnedString {
    return resolve(?maplibre.OwnedString, try map.copyStyleSourceUrl(testing.allocator, source_id));
}

pub fn styleSourceTileUrls(map: *maplibre.MapHandle, source_id: []const u8) !?maplibre.StringList {
    return resolve(?maplibre.StringList, try map.getStyleSourceTileUrls(testing.allocator, source_id));
}

/// The layer's style-spec type name, a static string the process owns.
pub fn styleLayerType(map: *maplibre.MapHandle, layer_id: []const u8) !?[]const u8 {
    var info = (try styleLayerInfo(map, layer_id)) orelse return null;
    defer info.deinit();
    return info.layer_type;
}

pub fn styleLayerJson(map: *maplibre.MapHandle, layer_id: []const u8) !?maplibre.OwnedString {
    return resolve(?maplibre.OwnedString, try map.getStyleLayerJson(testing.allocator, layer_id));
}

pub fn styleImageInfo(map: *maplibre.MapHandle, image_id: []const u8) !?maplibre.StyleImageInfo {
    return resolve(?maplibre.StyleImageInfo, try map.getStyleImageInfo(testing.allocator, image_id));
}

pub fn styleImageStretches(map: *maplibre.MapHandle, image_id: []const u8) !?maplibre.OwnedImageStretches {
    return resolve(?maplibre.OwnedImageStretches, try map.copyStyleImageStretches(testing.allocator, image_id));
}

pub fn imageSourceCoordinates(map: *maplibre.MapHandle, source_id: []const u8) !?[4]maplibre.LatLng {
    return resolve(?[4]maplibre.LatLng, try map.getImageSourceCoordinates(testing.allocator, source_id));
}

pub fn layerSourceLayer(map: *maplibre.MapHandle, layer_id: []const u8) !maplibre.OwnedString {
    return resolve(maplibre.OwnedString, try map.copyLayerSourceLayer(testing.allocator, layer_id));
}

pub fn layerSourceId(map: *maplibre.MapHandle, layer_id: []const u8) !maplibre.OwnedString {
    return resolve(maplibre.OwnedString, try map.copyLayerSourceId(testing.allocator, layer_id));
}

pub fn layerProperty(map: *maplibre.MapHandle, layer_id: []const u8, property_name: []const u8) !?maplibre.OwnedString {
    return resolve(?maplibre.OwnedString, try map.getLayerProperty(testing.allocator, layer_id, property_name));
}

pub fn layerFilter(map: *maplibre.MapHandle, layer_id: []const u8) !?maplibre.OwnedString {
    return resolve(?maplibre.OwnedString, try map.getLayerFilter(testing.allocator, layer_id));
}

pub fn styleLightProperty(map: *maplibre.MapHandle, property_name: []const u8) !?maplibre.OwnedString {
    return resolve(?maplibre.OwnedString, try map.getStyleLightProperty(testing.allocator, property_name));
}

/// Existence via the info getter's found flag.
pub fn styleImageExists(map: *maplibre.MapHandle, image_id: []const u8) !bool {
    return (try styleImageInfo(map, image_id)) != null;
}

pub fn styleImagePixels(map: *maplibre.MapHandle, image_id: []const u8) !?maplibre.OwnedString {
    return resolve(?maplibre.OwnedString, try map.copyStyleImagePremultipliedRgba8(testing.allocator, image_id));
}

pub fn styleTransitionOptions(map: *maplibre.MapHandle) !maplibre.StyleTransitionOptions {
    return resolve(maplibre.StyleTransitionOptions, try map.getStyleTransitionOptions());
}

pub fn listIndexOf(list: maplibre.StringList, value: []const u8) ?usize {
    for (list.items, 0..) |item, index| {
        if (std.mem.eql(u8, item, value)) return index;
    }
    return null;
}

pub fn expectListContains(list: maplibre.StringList, value: []const u8) !void {
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
    session: maplibre.RenderSessionHandle,
    future_value: maplibre.Future(T),
    service_driver: bool,
) !T {
    var future = future_value;
    defer future.deinit();
    for (0..wait_turns) |turn| {
        if (try future.poll()) return future.wait(null);
        if (service_driver) _ = session.serviceDriverWork(64) catch 0;
        try waitOneTurn(turn);
    }
    return error.OperationTimedOut;
}

pub fn finishOperation(
    session: maplibre.RenderSessionHandle,
    future: maplibre.Future(void),
    service_driver: bool,
) !void {
    _ = try resolveSessionFuture(void, session, future, service_driver);
}

pub fn finishAttachment(
    attachment: maplibre.RenderSessionAttachment,
    service_driver: bool,
) !maplibre.RenderSessionHandle {
    errdefer {
        var session = attachment.session;
        session.destroy() catch {};
    }
    try finishOperation(attachment.session, attachment.attached, service_driver);
    return attachment.session;
}

pub fn closeSession(session: *maplibre.RenderSessionHandle, service_driver: bool) !void {
    try finishOperation(session.*, try session.detach(), service_driver);
    try session.destroy();
}

var next_frame_token: std.atomic.Value(u64) = .init(1);

pub fn nextFrameToken() u64 {
    return next_frame_token.fetchAdd(1, .seq_cst);
}

/// Requests one frame carrying `demand` and returns the result for its token.
pub fn renderFrameWithDemand(
    session: maplibre.RenderSessionHandle,
    demand: maplibre.FrameDemand,
    service_driver: bool,
) !maplibre.FrameResult {
    try session.requestFrame(demand);
    for (0..wait_turns) |turn| {
        if (service_driver) _ = session.serviceDriverWork(64) catch 0;
        var batch = try session.drainFrameResults(testing.allocator);
        defer batch.deinit();
        for (0..batch.len()) |index| {
            const result = try batch.at(index);
            if (result.token == demand.token) return result;
        }
        try waitOneTurn(turn);
    }
    return error.FrameTimedOut;
}

/// Requests one presenting frame and returns its result.
pub fn renderFrame(
    session: maplibre.RenderSessionHandle,
    if_needed: bool,
    service_driver: bool,
) !maplibre.FrameResult {
    const capabilities = try session.capabilities();
    return renderFrameWithDemand(session, .{
        .if_needed = if_needed,
        .present = capabilities.presentation,
        .token = nextFrameToken(),
    }, service_driver);
}

/// Renders until one frame reports `.rendered`, tolerating the pending
/// dispositions a resize or a target swap produces.
pub fn expectRenderedFrame(
    session: maplibre.RenderSessionHandle,
    service_driver: bool,
) !maplibre.FrameResult {
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
