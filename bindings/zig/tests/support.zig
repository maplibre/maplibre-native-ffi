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
fn resolve(comptime T: type, future_value: maplibre.Future(T)) !T {
    var future = future_value;
    defer future.deinit();
    return future.wait(null);
}

pub fn waitForCommandCompletion(
    runtime: *maplibre.RuntimeHandle,
    future: maplibre.Future(maplibre.CommandCompletion),
) !maplibre.CommandCompletion {
    _ = runtime;
    return resolve(maplibre.CommandCompletion, future);
}

pub fn waitForCommandDisposition(
    runtime: *maplibre.RuntimeHandle,
    future: maplibre.Future(maplibre.CommandCompletion),
) !maplibre.CommandDisposition {
    return (try waitForCommandCompletion(runtime, future)).disposition;
}

/// Verifies that an accepted command completed with a native failure.
pub fn expectCommandError(
    runtime: *maplibre.RuntimeHandle,
    future_value: maplibre.Future(maplibre.CommandCompletion),
    expected: anyerror,
) !void {
    _ = runtime;
    var future = future_value;
    defer future.deinit();
    const completion = try future.wait(null);
    try testing.expectEqual(maplibre.CommandDisposition.failed, completion.disposition);
    try testing.expectError(expected, completion.statusError());
    try testing.expect((try future.diagnostic()).len != 0);
}

/// Awaits `completion`'s commit and returns a map snapshot that observes it:
/// the committed event reports the published generation, and a snapshot at or
/// past that generation carries the committed state.
pub fn snapshotAfterCommand(
    runtime: *maplibre.RuntimeHandle,
    map: *maplibre.MapHandle,
    future: maplibre.Future(maplibre.CommandCompletion),
) !maplibre.MapSnapshot {
    const finished = try waitForCommandCompletion(runtime, future);
    try testing.expect(std.meta.eql(finished.disposition, maplibre.CommandDisposition.committed));
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
    var map_future = try maplibre.MapHandle.create(runtime, .{});
    defer map_future.deinit();
    var map = try map_future.wait(null);
    errdefer map.close() catch {};
    _ = try resolve(maplibre.CommandCompletion, try map.setStyleJson(testing.allocator, style_json));
    try testing.expect(try waitForEvent(runtime, .map_style_loaded));
    return map;
}
pub fn waitForBarrier(runtime: *maplibre.RuntimeHandle) !void {
    _ = try resolve(void, try runtime.barrier());
}

/// Copies fixed source metadata, reporting null when no source has the ID.
pub fn styleSourceMetadata(map: *maplibre.MapHandle, source_id: []const u8) !?maplibre.StyleSourceMetadata {
    return resolve(?maplibre.StyleSourceMetadata, try map.getStyleSourceInfo(testing.allocator, source_id));
}

/// Existence via the info getter's found flag.
pub fn styleSourceExists(map: *maplibre.MapHandle, source_id: []const u8) !bool {
    return (try styleSourceMetadata(map, source_id)) != null;
}

pub fn styleSourceType(map: *maplibre.MapHandle, source_id: []const u8) !?maplibre.StyleSourceType {
    const metadata = (try styleSourceMetadata(map, source_id)) orelse return null;
    return metadata.source_type;
}

/// Waits for a removal command: true when it commits, false when the ID names
/// nothing, and the command's reported failure otherwise.
fn awaitRemoval(runtime: *maplibre.RuntimeHandle, future: maplibre.Future(maplibre.CommandCompletion)) !bool {
    _ = runtime;
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
    return resolve(?maplibre.StyleLayerInfo, try map.getStyleLayerInfo(testing.allocator, layer_id));
}

/// Existence via the info getter's found flag.
pub fn styleLayerExists(map: *maplibre.MapHandle, layer_id: []const u8) !bool {
    return (try styleLayerInfo(map, layer_id)) != null;
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
        tile_urls = try resolve(maplibre.StringList, try map.getStyleSourceTileUrls(testing.allocator, source_id));
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

fn sleepOneMillisecond() !void {
    try testing.io.sleep(.fromMilliseconds(1), .awake);
}
