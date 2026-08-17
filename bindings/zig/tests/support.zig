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

/// Waits for an autonomously produced event of `event_type`, reporting whether
/// it arrived.
///
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
/// Waits for `command_id`'s terminal event and returns its payload. See
/// `waitForEvent` for its polling behavior.
pub fn waitForCommandFinished(
    runtime: *maplibre.RuntimeHandle,
    command_id: u64,
) !maplibre.CommandFinishedPayload {
    for (0..5000) |_| {
        var batch = try runtime.drainEvents(testing.allocator);
        defer batch.deinit();
        for (0..batch.len()) |index| {
            const event = try batch.at(index);
            switch (event.payload) {
                .command_finished => |payload| {
                    if (payload.command_id == command_id) return payload;
                },
                else => {},
            }
        }
        try sleepOneMillisecond();
    }
    return error.EventNotObserved;
}

pub fn waitForCommandDisposition(
    runtime: *maplibre.RuntimeHandle,
    command_id: u64,
) !maplibre.CommandDisposition {
    return (try waitForCommandFinished(runtime, command_id)).disposition;
}

/// Awaits `command_id`'s commit and returns a map snapshot that observes it:
/// the committed event reports the published generation, and a snapshot at or
/// past that generation carries the committed state.
pub fn snapshotAfterCommand(
    runtime: *maplibre.RuntimeHandle,
    map: *maplibre.MapHandle,
    command_id: u64,
) !maplibre.MapSnapshot {
    const finished = try waitForCommandFinished(runtime, command_id);
    try testing.expect(std.meta.eql(finished.disposition, maplibre.CommandDisposition.committed));
    try finished.status;
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
    var map = try maplibre.MapHandle.create(runtime, .{});
    errdefer map.close() catch {};
    _ = try map.setStyleJson(testing.allocator, style_json);
    try testing.expect(try waitForEvent(runtime, .map_style_loaded));
    return map;
}
pub fn waitOperation(operation: maplibre.OperationHandle) !void {
    try testing.expect(try operation.waitForSuccess(-1));
}
pub fn waitForBarrier(runtime: *maplibre.RuntimeHandle) !void {
    const operation = try runtime.barrierStart();
    defer operation.release();
    try waitOperation(operation);
    try operation.discard();
}

/// Copies fixed source metadata, reporting null when no source has the ID.
pub fn styleSourceMetadata(map: *maplibre.MapHandle, source_id: []const u8) !?maplibre.StyleSourceMetadata {
    const operation = try map.getStyleSourceInfoStart(testing.allocator, source_id);
    defer operation.release();
    try waitOperation(operation);
    return map.getStyleSourceInfoTakeResult(operation);
}

/// Existence via the info getter's found flag.
pub fn styleSourceExists(map: *maplibre.MapHandle, source_id: []const u8) !bool {
    return (try styleSourceMetadata(map, source_id)) != null;
}

pub fn styleSourceType(map: *maplibre.MapHandle, source_id: []const u8) !?maplibre.StyleSourceType {
    const metadata = (try styleSourceMetadata(map, source_id)) orelse return null;
    return metadata.source_type;
}

/// Waits for a removal command's terminal event: true when the removal
/// committed, false when the ID named nothing, and the command's reported
/// failure otherwise.
fn awaitRemoval(runtime: *maplibre.RuntimeHandle, command_id: u64) !bool {
    const payload = try waitForCommandFinished(runtime, command_id);
    switch (payload.disposition) {
        .committed => return true,
        .failed => {
            payload.status catch |err| {
                if (err == error.NotFound) return false;
                return err;
            };
            return error.UnexpectedCommandStatus;
        },
        else => return error.UnexpectedCommandDisposition,
    }
}

pub fn removeStyleSource(runtime: *maplibre.RuntimeHandle, map: *maplibre.MapHandle, source_id: []const u8) !bool {
    return awaitRemoval(runtime, try map.removeStyleSource(testing.allocator, source_id));
}

pub fn removeStyleLayer(runtime: *maplibre.RuntimeHandle, map: *maplibre.MapHandle, layer_id: []const u8) !bool {
    return awaitRemoval(runtime, try map.removeStyleLayer(testing.allocator, layer_id));
}

pub fn removeStyleImage(runtime: *maplibre.RuntimeHandle, map: *maplibre.MapHandle, image_id: []const u8) !bool {
    return awaitRemoval(runtime, try map.removeStyleImage(testing.allocator, image_id));
}

/// Copies fixed layer metadata, reporting null when no layer has the ID.
pub fn styleLayerInfo(map: *maplibre.MapHandle, layer_id: []const u8) !?maplibre.StyleLayerInfo {
    const operation = try map.getStyleLayerInfoStart(testing.allocator, layer_id);
    defer operation.release();
    try waitOperation(operation);
    return map.getStyleLayerInfoTakeResult(operation);
}

/// Existence via the info getter's found flag.
pub fn styleLayerExists(map: *maplibre.MapHandle, layer_id: []const u8) !bool {
    return (try styleLayerInfo(map, layer_id)) != null;
}

pub fn loadedStyleJson(map: *maplibre.MapHandle) !maplibre.OwnedString {
    const operation = try map.loadedStyleJsonStart();
    defer operation.release();
    try waitOperation(operation);
    return map.loadedStyleJsonTakeResult(testing.allocator, operation);
}

pub fn styleUrl(map: *maplibre.MapHandle) !maplibre.OwnedString {
    const operation = try map.styleUrlStart();
    defer operation.release();
    try waitOperation(operation);
    return map.styleUrlTakeResult(testing.allocator, operation);
}

pub fn listStyleSourceIds(map: *maplibre.MapHandle) !maplibre.StringList {
    const operation = try map.listStyleSourceIdsStart();
    defer operation.release();
    try waitOperation(operation);
    return map.listStyleSourceIdsTakeResult(testing.allocator, operation);
}

pub fn listStyleLayerIds(map: *maplibre.MapHandle) !maplibre.StringList {
    const operation = try map.listStyleLayerIdsStart();
    defer operation.release();
    try waitOperation(operation);
    return map.listStyleLayerIdsTakeResult(testing.allocator, operation);
}

pub fn styleSourceAttribution(map: *maplibre.MapHandle, source_id: []const u8) !?maplibre.OwnedString {
    const operation = try map.copyStyleSourceAttributionStart(testing.allocator, source_id);
    defer operation.release();
    try waitOperation(operation);
    return map.copyStyleSourceAttributionTakeResult(testing.allocator, operation);
}

pub fn styleSourceUrl(map: *maplibre.MapHandle, source_id: []const u8) !?maplibre.OwnedString {
    const operation = try map.copyStyleSourceUrlStart(testing.allocator, source_id);
    defer operation.release();
    try waitOperation(operation);
    return map.copyStyleSourceUrlTakeResult(testing.allocator, operation);
}

pub fn styleSourceInfo(map: *maplibre.MapHandle, source_id: []const u8) !?maplibre.StyleSourceInfo {
    const metadata = (try styleSourceMetadata(map, source_id)) orelse return null;
    var attribution: ?[]const u8 = null;
    errdefer if (attribution) |value| testing.allocator.free(value);
    if (metadata.has_attribution) {
        var owned = (try styleSourceAttribution(map, source_id)) orelse return error.NativeError;
        attribution = owned.value;
        owned.value = "";
    }
    var url: ?[]const u8 = null;
    errdefer if (url) |value| testing.allocator.free(value);
    if (metadata.has_url) {
        var owned = (try styleSourceUrl(map, source_id)) orelse return error.NativeError;
        url = owned.value;
        owned.value = "";
    }
    var tile_urls: ?maplibre.StringList = null;
    errdefer if (tile_urls) |*list| list.deinit();
    if (metadata.has_tile_json) {
        const tiles_operation = try map.getStyleSourceTileUrlsStart(testing.allocator, source_id);
        defer tiles_operation.release();
        try waitOperation(tiles_operation);
        tile_urls = (try map.getStyleSourceTileUrlsTakeResult(testing.allocator, tiles_operation)) orelse return error.NativeError;
    }
    return .{
        .allocator = testing.allocator,
        .source_type = metadata.source_type,
        .id_size = metadata.id_size,
        .is_volatile = metadata.is_volatile,
        .attribution = attribution,
        .url = url,
        .tile_json = if (tile_urls) |list| .{
            .tile_urls = list.items,
            .min_zoom = metadata.min_zoom,
            .max_zoom = metadata.max_zoom,
            .scheme = metadata.scheme,
            .bounds = metadata.bounds,
        } else null,
        .tile_size = metadata.tile_size,
        .vector_encoding = metadata.vector_encoding,
        .raster_encoding = metadata.raster_encoding,
    };
}

/// The layer's style-spec type name, a static string the process owns.
pub fn styleLayerType(map: *maplibre.MapHandle, layer_id: []const u8) !?[]const u8 {
    const info = (try styleLayerInfo(map, layer_id)) orelse return null;
    return info.layer_type;
}

pub fn styleLayerJson(map: *maplibre.MapHandle, layer_id: []const u8) !?maplibre.OwnedString {
    const operation = try map.getStyleLayerJsonStart(testing.allocator, layer_id);
    defer operation.release();
    try waitOperation(operation);
    return map.getStyleLayerJsonTakeResult(testing.allocator, operation);
}

pub fn styleImageInfo(map: *maplibre.MapHandle, image_id: []const u8) !?maplibre.StyleImageInfo {
    const operation = try map.getStyleImageInfoStart(testing.allocator, image_id);
    defer operation.release();
    try waitOperation(operation);
    return map.getStyleImageInfoTakeResult(operation);
}

pub fn styleImageStretches(map: *maplibre.MapHandle, image_id: []const u8) !?maplibre.OwnedImageStretches {
    const operation = try map.copyStyleImageStretchesStart(testing.allocator, image_id);
    defer operation.release();
    try waitOperation(operation);
    return map.copyStyleImageStretchesTakeResult(testing.allocator, operation);
}

pub fn imageSourceCoordinates(map: *maplibre.MapHandle, source_id: []const u8) !?[4]maplibre.LatLng {
    const operation = try map.getImageSourceCoordinatesStart(testing.allocator, source_id);
    defer operation.release();
    try waitOperation(operation);
    return map.getImageSourceCoordinatesTakeResult(operation);
}

pub fn layerSourceLayer(map: *maplibre.MapHandle, layer_id: []const u8) !maplibre.OwnedString {
    const operation = try map.copyLayerSourceLayerStart(testing.allocator, layer_id);
    defer operation.release();
    try waitOperation(operation);
    return map.copyLayerSourceLayerTakeResult(testing.allocator, operation);
}

pub fn layerSourceId(map: *maplibre.MapHandle, layer_id: []const u8) !maplibre.OwnedString {
    const operation = try map.copyLayerSourceIdStart(testing.allocator, layer_id);
    defer operation.release();
    try waitOperation(operation);
    return map.copyLayerSourceIdTakeResult(testing.allocator, operation);
}

pub fn layerProperty(map: *maplibre.MapHandle, layer_id: []const u8, property_name: []const u8) !?maplibre.OwnedString {
    const operation = try map.getLayerPropertyStart(testing.allocator, layer_id, property_name);
    defer operation.release();
    try waitOperation(operation);
    return map.getLayerPropertyTakeResult(testing.allocator, operation);
}

pub fn layerFilter(map: *maplibre.MapHandle, layer_id: []const u8) !?maplibre.OwnedString {
    const operation = try map.getLayerFilterStart(testing.allocator, layer_id);
    defer operation.release();
    try waitOperation(operation);
    return map.getLayerFilterTakeResult(testing.allocator, operation);
}

pub fn styleLightProperty(map: *maplibre.MapHandle, property_name: []const u8) !?maplibre.OwnedString {
    const operation = try map.getStyleLightPropertyStart(testing.allocator, property_name);
    defer operation.release();
    try waitOperation(operation);
    return map.getStyleLightPropertyTakeResult(testing.allocator, operation);
}

/// Existence via the info getter's found flag.
pub fn styleImageExists(map: *maplibre.MapHandle, image_id: []const u8) !bool {
    return (try styleImageInfo(map, image_id)) != null;
}

pub fn styleImagePixels(map: *maplibre.MapHandle, image_id: []const u8) !?maplibre.OwnedString {
    const operation = try map.copyStyleImagePremultipliedRgba8Start(testing.allocator, image_id);
    defer operation.release();
    try waitOperation(operation);
    return map.copyStyleImagePremultipliedRgba8TakeResult(testing.allocator, operation);
}

pub fn styleTransitionOptions(map: *maplibre.MapHandle) !maplibre.StyleTransitionOptions {
    const operation = try map.getStyleTransitionOptionsStart();
    defer operation.release();
    try waitOperation(operation);
    return map.getStyleTransitionOptionsTakeResult(operation);
}

fn sleepOneMillisecond() !void {
    try testing.io.sleep(.fromMilliseconds(1), .awake);
}
