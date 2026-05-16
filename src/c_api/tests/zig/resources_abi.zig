// Raw C ABI coverage: null handles/outputs, unknown raw operation values, null paths, callback descriptor shape, and invalid raw offline descriptors are hidden by the Zig binding.

const std = @import("std");
const testing = std.testing;
const support = @import("support.zig");
const c = support.c;

const offline_style_url = "http://example.com/offline-style.json";

fn offlineTileDefinition() c.mln_offline_region_definition {
    var definition: c.mln_offline_region_definition = undefined;
    definition.size = @sizeOf(c.mln_offline_region_definition);
    definition.type = c.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID;
    definition.data.tile_pyramid = .{
        .size = @sizeOf(c.mln_offline_tile_pyramid_region_definition),
        .style_url = offline_style_url,
        .bounds = .{
            .southwest = .{ .latitude = 1.0, .longitude = 2.0 },
            .northeast = .{ .latitude = 3.0, .longitude = 4.0 },
        },
        .min_zoom = 5.0,
        .max_zoom = 6.0,
        .pixel_ratio = 2.0,
        .include_ideographs = true,
    };
    return definition;
}

fn offlineGeometryDefinition(geometry: *const c.mln_geometry) c.mln_offline_region_definition {
    var definition: c.mln_offline_region_definition = undefined;
    definition.size = @sizeOf(c.mln_offline_region_definition);
    definition.type = c.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY;
    definition.data.geometry = .{
        .size = @sizeOf(c.mln_offline_geometry_region_definition),
        .style_url = offline_style_url,
        .geometry = geometry,
        .min_zoom = 5.0,
        .max_zoom = 6.0,
        .pixel_ratio = 2.0,
        .include_ideographs = true,
    };
    return definition;
}

fn styleResponse() c.mln_resource_response {
    return .{
        .size = @sizeOf(c.mln_resource_response),
        .status = c.MLN_RESOURCE_RESPONSE_STATUS_OK,
        .error_reason = c.MLN_RESOURCE_ERROR_REASON_NONE,
        .bytes = null,
        .byte_count = 0,
        .error_message = null,
        .must_revalidate = false,
        .has_modified = false,
        .modified_unix_ms = 0,
        .has_expires = false,
        .expires_unix_ms = 0,
        .etag = null,
        .has_retry_after = false,
        .retry_after_unix_ms = 0,
    };
}

fn resourceProviderStub(_: ?*anyopaque, _: [*c]const c.mln_resource_request, _: ?*c.mln_resource_request_handle) callconv(.c) u32 {
    return c.MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
}

fn resourceTransformStub(_: ?*anyopaque, _: u32, _: [*c]const u8, out_response: [*c]c.mln_resource_transform_response) callconv(.c) c.mln_status {
    if (out_response == null) return c.MLN_STATUS_INVALID_ARGUMENT;
    out_response.*.url = null;
    return c.MLN_STATUS_OK;
}

test "custom provider request handles reject raw null handles" {
    c.mln_resource_request_release(null);

    var cancelled = true;
    var response = styleResponse();
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_resource_request_cancelled(null, &cancelled));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_resource_request_complete(null, &response));
}

test "network status get rejects raw null output" {
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_network_status_get(null));
}

test "ambient cache operations validate raw operation values" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_run_ambient_cache_operation(runtime, 999));
}

test "offline regions reject raw invalid descriptors" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);

    var definition = offlineTileDefinition();
    const metadata = [_]u8{ 1, 2, 3 };
    var snapshot: ?*c.mln_offline_region_snapshot = null;

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_offline_region_create(runtime, null, metadata[0..].ptr, metadata.len, &snapshot));
    try testing.expectEqual(@as(?*c.mln_offline_region_snapshot, null), snapshot);

    definition.type = 999;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_offline_region_create(runtime, &definition, metadata[0..].ptr, metadata.len, &snapshot));
    try testing.expectEqual(@as(?*c.mln_offline_region_snapshot, null), snapshot);

    definition = offlineTileDefinition();
    definition.data.tile_pyramid.style_url = null;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_offline_region_create(runtime, &definition, metadata[0..].ptr, metadata.len, &snapshot));
    try testing.expectEqual(@as(?*c.mln_offline_region_snapshot, null), snapshot);

    var coordinates = [_]c.mln_lat_lng{
        .{ .latitude = 1.0, .longitude = 2.0 },
        .{ .latitude = 3.0, .longitude = 4.0 },
    };
    var geometry = c.mln_geometry{
        .size = @sizeOf(c.mln_geometry),
        .type = c.MLN_GEOMETRY_TYPE_LINE_STRING,
        .data = .{ .line_string = .{ .coordinates = coordinates[0..].ptr, .coordinate_count = coordinates.len } },
    };
    definition = offlineGeometryDefinition(&geometry);
    definition.data.geometry.style_url = null;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_offline_region_create(runtime, &definition, metadata[0..].ptr, metadata.len, &snapshot));
    try testing.expectEqual(@as(?*c.mln_offline_region_snapshot, null), snapshot);

    definition = offlineGeometryDefinition(&geometry);
    definition.data.geometry.geometry = null;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_offline_region_create(runtime, &definition, metadata[0..].ptr, metadata.len, &snapshot));
    try testing.expectEqual(@as(?*c.mln_offline_region_snapshot, null), snapshot);
}

test "offline database merge rejects raw null path" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);

    var merged: ?*c.mln_offline_region_list = null;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_offline_regions_merge_database(runtime, null, &merged));
}

test "resource transform rejects raw invalid descriptors" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_clear_resource_transform(null));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_set_resource_transform(runtime, null));
    var transform = c.mln_resource_transform{ .size = 0, .callback = resourceTransformStub, .user_data = null };
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_set_resource_transform(runtime, &transform));
    transform.size = @sizeOf(c.mln_resource_transform);
    transform.callback = null;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_set_resource_transform(runtime, &transform));
}

test "resource transform updates and clears after map creation" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);

    var transform = c.mln_resource_transform{ .size = @sizeOf(c.mln_resource_transform), .callback = resourceTransformStub, .user_data = null };
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_runtime_set_resource_transform(runtime, &transform));

    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_runtime_set_resource_transform(runtime, &transform));
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_runtime_clear_resource_transform(runtime));
}

test "resource provider rejects raw invalid descriptors" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_set_resource_provider(runtime, null));

    var provider = c.mln_resource_provider{
        .size = 0,
        .callback = resourceProviderStub,
        .user_data = null,
    };
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_set_resource_provider(runtime, &provider));

    provider.size = @sizeOf(c.mln_resource_provider);
    provider.callback = null;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_set_resource_provider(runtime, &provider));
}
