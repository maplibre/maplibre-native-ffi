// Raw C ABI coverage: style image/source/layer functions expose null output buffers, undersized counts, and unknown raw enum values that the Zig binding cannot construct.

const std = @import("std");
const testing = std.testing;
const support = @import("support.zig");
const c = support.c;

const style_json =
    \\{
    \\  "version": 8,
    \\  "name": "zig-raw-style-abi-test",
    \\  "sources": {},
    \\  "layers": [
    \\    {"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}}
    \\  ]
    \\}
;

fn sleepOneMillisecond() !void {
    try testing.io.sleep(.fromMilliseconds(1), .awake);
}

fn stringView(value: []const u8) c.mln_string_view {
    return .{ .data = value.ptr, .size = value.len };
}

fn emptyEvent() c.mln_runtime_event {
    return .{
        .size = @sizeOf(c.mln_runtime_event),
        .type = 0,
        .source_type = c.MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
        .source = null,
        .code = 0,
        .payload_type = c.MLN_RUNTIME_EVENT_PAYLOAD_NONE,
        .payload = null,
        .payload_size = 0,
        .message = null,
        .message_size = 0,
    };
}

fn waitForMapEvent(runtime: *c.mln_runtime, map: *c.mln_map, event_type: u32) !bool {
    for (0..1000) |_| {
        try testing.expectEqual(c.MLN_STATUS_OK, c.mln_runtime_run_once(runtime));
        while (true) {
            var event = emptyEvent();
            var has_event = false;
            try testing.expectEqual(c.MLN_STATUS_OK, c.mln_runtime_poll_event(runtime, &event, &has_event));
            if (!has_event) break;
            if (event.type == event_type and event.source_type == c.MLN_RUNTIME_EVENT_SOURCE_MAP and event.source == @as(?*anyopaque, @ptrCast(map))) return true;
        }
        try sleepOneMillisecond();
    }
    return false;
}

test "style value helpers reject unsafe raw descriptors" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_set_style_json(map, style_json));
    try testing.expect(try waitForMapEvent(runtime, map, c.MLN_RUNTIME_EVENT_MAP_STYLE_LOADED));

    var image_pixels = [_]u8{ 1, 2, 3, 4 };
    var image = c.mln_premultiplied_rgba8_image_default();
    image.width = 1;
    image.height = 1;
    image.stride = 4;
    image.pixels = &image_pixels;
    image.byte_length = image_pixels.len;

    var image_options = c.mln_style_image_options_default();
    image_options.fields = c.MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO | c.MLN_STYLE_IMAGE_OPTION_SDF;
    image_options.pixel_ratio = 2.0;
    image_options.sdf = true;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_set_style_image(map, stringView("runtime-icon"), &image, &image_options));

    var required: usize = 0;
    var found = false;
    try testing.expectEqual(
        c.MLN_STATUS_INVALID_ARGUMENT,
        c.mln_map_copy_style_image_premultiplied_rgba8(map, stringView("runtime-icon"), null, 0, &required, &found),
    );
    try testing.expect(found);
    try testing.expectEqual(@as(usize, 4), required);

    var coordinates = [_]c.mln_lat_lng{
        .{ .latitude = 38.0, .longitude = -123.0 },
        .{ .latitude = 38.0, .longitude = -122.0 },
        .{ .latitude = 37.0, .longitude = -122.0 },
        .{ .latitude = 37.0, .longitude = -123.0 },
    };
    try testing.expectEqual(
        c.MLN_STATUS_OK,
        c.mln_map_add_image_source_url(map, stringView("image-url-source"), &coordinates, coordinates.len, stringView("https://example.com/image.png")),
    );

    var required_coordinates: usize = 0;
    try testing.expectEqual(
        c.MLN_STATUS_INVALID_ARGUMENT,
        c.mln_map_get_image_source_coordinates(map, stringView("image-url-source"), null, 0, &required_coordinates, &found),
    );
    try testing.expect(found);
    try testing.expectEqual(@as(usize, 4), required_coordinates);
    try testing.expectEqual(
        c.MLN_STATUS_INVALID_ARGUMENT,
        c.mln_map_set_image_source_coordinates(map, stringView("image-url-source"), &coordinates, 3),
    );

    var tile_options = c.mln_style_tile_source_options_default();
    tile_options.fields = c.MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
    tile_options.raster_encoding = 99;
    try testing.expectEqual(
        c.MLN_STATUS_INVALID_ARGUMENT,
        c.mln_map_add_raster_dem_source_url(map, stringView("bad-dem"), stringView("https://example.com/bad.json"), &tile_options),
    );

    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_add_location_indicator_layer(map, stringView("location"), stringView("")));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_location_indicator_image_name(map, stringView("location"), 99, stringView("bad")));
}
