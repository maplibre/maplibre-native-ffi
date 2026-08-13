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
/// A wait that stops at the event it looks for takes one event per drain, so the
/// events a later wait needs stay queued rather than being dropped with the
/// batch that carried them.
pub fn waitForEvent(runtime: *maplibre.RuntimeHandle, event_type: maplibre.RuntimeEventType) !bool {
    for (0..1000) |_| {
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
pub fn waitForCommandDisposition(
    runtime: *maplibre.RuntimeHandle,
    command_id: u64,
) !maplibre.CommandDisposition {
    for (0..5000) |_| {
        while (true) {
            var batch = try runtime.drainEvents(testing.allocator, 1);
            defer batch.deinit();
            if (batch.len() == 0) break;
            const event = try batch.at(0);
            switch (event.payload) {
                .command_finished => |payload| {
                    if (payload.command_id == command_id) return payload.disposition;
                },
                else => {},
            }
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

pub fn styleSourceExists(map: *maplibre.MapHandle, source_id: []const u8) !bool {
    const operation = try map.styleSourceExistsStart(testing.allocator, source_id);
    defer operation.release();
    try waitOperation(operation);
    return map.styleSourceExistsTakeResult(operation);
}

pub fn styleSourceType(map: *maplibre.MapHandle, source_id: []const u8) !?maplibre.StyleSourceType {
    const operation = try map.getStyleSourceTypeStart(testing.allocator, source_id);
    defer operation.release();
    try waitOperation(operation);
    return map.getStyleSourceTypeTakeResult(operation);
}

pub fn removeStyleSource(map: *maplibre.MapHandle, source_id: []const u8) !bool {
    const operation = try map.removeStyleSourceStart(testing.allocator, source_id);
    defer operation.release();
    try waitOperation(operation);
    return map.removeStyleSourceTakeResult(operation);
}

pub fn styleLayerExists(map: *maplibre.MapHandle, layer_id: []const u8) !bool {
    const operation = try map.styleLayerExistsStart(testing.allocator, layer_id);
    defer operation.release();
    try waitOperation(operation);
    return map.styleLayerExistsTakeResult(operation);
}

pub fn removeStyleLayer(map: *maplibre.MapHandle, layer_id: []const u8) !bool {
    const operation = try map.removeStyleLayerStart(testing.allocator, layer_id);
    defer operation.release();
    try waitOperation(operation);
    return map.removeStyleLayerTakeResult(operation);
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
    const operation = try map.getStyleSourceInfoStart(testing.allocator, source_id);
    defer operation.release();
    try waitOperation(operation);
    const metadata = (try map.getStyleSourceInfoTakeResult(operation)) orelse return null;
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

pub fn styleLayerType(map: *maplibre.MapHandle, layer_id: []const u8) !?maplibre.OwnedString {
    const operation = try map.getStyleLayerTypeStart(testing.allocator, layer_id);
    defer operation.release();
    try waitOperation(operation);
    return map.getStyleLayerTypeTakeResult(testing.allocator, operation);
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

pub fn layerMinZoom(map: *maplibre.MapHandle, layer_id: []const u8) !f64 {
    const operation = try map.getLayerMinZoomStart(testing.allocator, layer_id);
    defer operation.release();
    try waitOperation(operation);
    return map.getLayerMinZoomTakeResult(operation);
}

pub fn layerMaxZoom(map: *maplibre.MapHandle, layer_id: []const u8) !f64 {
    const operation = try map.getLayerMaxZoomStart(testing.allocator, layer_id);
    defer operation.release();
    try waitOperation(operation);
    return map.getLayerMaxZoomTakeResult(operation);
}

pub fn layerVisibility(map: *maplibre.MapHandle, layer_id: []const u8) !maplibre.StyleLayerVisibility {
    const operation = try map.getLayerVisibilityStart(testing.allocator, layer_id);
    defer operation.release();
    try waitOperation(operation);
    return map.getLayerVisibilityTakeResult(operation);
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

pub fn styleImageExists(map: *maplibre.MapHandle, image_id: []const u8) !bool {
    const operation = try map.styleImageExistsStart(testing.allocator, image_id);
    defer operation.release();
    try waitOperation(operation);
    return map.styleImageExistsTakeResult(operation);
}

pub fn removeStyleImage(map: *maplibre.MapHandle, image_id: []const u8) !bool {
    const operation = try map.removeStyleImageStart(testing.allocator, image_id);
    defer operation.release();
    try waitOperation(operation);
    return map.removeStyleImageTakeResult(operation);
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

pub fn typeNameContains(comptime T: type, comptime needle: []const u8) bool {
    if (std.mem.indexOf(u8, @typeName(T), needle) != null) return true;
    const info = @typeInfo(T);
    if (info != .@"struct") return false;
    inline for (info.@"struct".fields) |field| {
        if (std.mem.indexOf(u8, @typeName(field.type), needle) != null) return true;
    }
    return false;
}
