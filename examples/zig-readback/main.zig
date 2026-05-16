const std = @import("std");
const build_options = @import("build_options");
const maplibre = @import("maplibre_native");

const width = 512;
const height = 512;
const style_json =
    \\{
    \\  "version": 8,
    \\  "name": "zig-readback",
    \\  "sources": {},
    \\  "layers": [
    \\    {"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}}
    \\  ]
    \\}
;

pub fn main(init_args: std.process.Init) !void {
    const allocator = init_args.gpa;
    var args = try std.process.Args.Iterator.initAllocator(init_args.minimal.args, allocator);
    defer args.deinit();
    _ = args.skip();
    const output_path = args.next() orelse "map.ppm";

    try maplibre.setAsyncLogSeverityMask(.none, null);
    defer maplibre.setAsyncLogSeverityMask(.default, null) catch {};
    try logAndValidateRenderBackend();

    var runtime = try maplibre.RuntimeHandle.create(allocator, .{ .cache_path = ":memory:" }, null);
    defer runtime.close() catch {};

    var map = try maplibre.MapHandle.create(&runtime, .{
        .width = width,
        .height = height,
        .scale_factor = 1.0,
        .mode = .continuous,
    });
    defer map.close() catch {};

    var texture = try maplibre.attachOwnedTexture(&map, .{
        .extent = .{ .width = width, .height = height, .scale_factor = 1.0 },
    });
    defer texture.close() catch {};

    try setInitialCamera(&map);
    try map.setStyleJson(allocator, style_json);
    try renderTexture(&runtime, &map, &texture);

    var image = try texture.readPremultipliedRgba8(allocator);
    defer image.deinit();

    try writePpm(init_args.io, allocator, output_path, image.data, image.info);
    std.debug.print("wrote {s} ({d}x{d})\n", .{ output_path, image.info.width, image.info.height });
}

fn logAndValidateRenderBackend() !void {
    const support = maplibre.supportedRenderBackends();
    std.debug.print("native render backends: {s}\n", .{renderBackendSupportLabel(support)});
    if (build_options.supports_metal and !support.metal) return error.NativeRenderBackendMismatch;
    if (build_options.supports_vulkan and !support.vulkan) return error.NativeRenderBackendMismatch;
}

fn renderBackendSupportLabel(support: maplibre.RenderBackendSupport) []const u8 {
    if (support.metal and support.vulkan) return "metal,vulkan";
    if (support.metal) return "metal";
    if (support.vulkan) return "vulkan";
    return "none";
}

fn setInitialCamera(map: *maplibre.MapHandle) !void {
    try map.jumpTo(.{
        .center = .{ .latitude = 37.7749, .longitude = -122.4194 },
        .zoom = 13.0,
        .bearing = 12.0,
        .pitch = 30.0,
    });
}

fn renderTexture(
    runtime: *maplibre.RuntimeHandle,
    map: *maplibre.MapHandle,
    texture: *maplibre.RenderSessionHandle,
) !void {
    const map_id = try map.id();
    for (0..10_000) |_| {
        try runtime.runOnce();
        while (try runtime.pollEvent()) |event| {
            if (event.source_type != .map or event.source_id == null or !std.meta.eql(event.source_id.?, map_id)) continue;
            switch (event.event_type) {
                .map_loading_failed => return error.MapLoadingFailed,
                .map_render_error => return error.MapRenderFailed,
                else => {},
            }
        }

        texture.renderUpdate() catch |err| switch (err) {
            error.InvalidState => {
                std.Thread.yield() catch {};
                continue;
            },
            else => return err,
        };
        return;
    }
    return error.RenderTimedOut;
}

fn writePpm(
    io: std.Io,
    allocator: std.mem.Allocator,
    output_path: []const u8,
    rgba: []const u8,
    info: maplibre.TextureImageInfo,
) !void {
    const pixel_count = @as(usize, @intCast(info.width)) * @as(usize, @intCast(info.height));
    const rgb = try allocator.alloc(u8, pixel_count * 3);
    defer allocator.free(rgb);

    for (0..pixel_count) |index| {
        rgb[index * 3 + 0] = rgba[index * 4 + 0];
        rgb[index * 3 + 1] = rgba[index * 4 + 1];
        rgb[index * 3 + 2] = rgba[index * 4 + 2];
    }

    var file = try std.Io.Dir.cwd().createFile(io, output_path, .{});
    defer file.close(io);

    var header_buffer: [64]u8 = undefined;
    const header = try std.fmt.bufPrint(
        &header_buffer,
        "P6\n{d} {d}\n255\n",
        .{ info.width, info.height },
    );
    try file.writeStreamingAll(io, header);
    try file.writeStreamingAll(io, rgb);
}
