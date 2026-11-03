//! Snippets for the docs site's Render to a texture page. They compile with the
//! tests, which never run them.

const std = @import("std");
const maplibre = @import("maplibre_native_ffi");

// Targets without Vulkan, such as Mac Catalyst, have no Vulkan attach methods.
comptime {
    if (@TypeOf(maplibre.Map.attachVulkanBorrowedTexture) != void) _ = &renderToTexture;
}

var session: maplibre.RenderSession = undefined;

// The frame wake from the Render to a window page.
fn onFrameWake(_: ?*anyopaque) anyerror!void {}

// Stand-ins for the host's mutex on its graphics queue.
fn lockQueue(_: ?*anyopaque) anyerror!void {}

fn unlockQueue(_: ?*anyopaque) anyerror!void {}

// The host's draw, which samples the frame's image view.
fn drawMap(_: void, texture: maplibre.VulkanTextureFrame) void {
    _ = texture.image_view;
}

fn destroyTextures(images: [2]u64, views: [2]u64) void {
    _ = .{ images, views };
}

fn drawFrame() !void {
    // #region acquire
    // Return when there's no new frame yet.
    var frame = (try session.acquireFrame(null)) orelse return;

    // Sample the texture, and wait for those reads to finish.
    try frame.getVulkanTexture({}, drawMap, null);

    try frame.release(.{ .kind = .cpu_complete }, null);
    // #endregion acquire
}

fn renderToTexture(
    allocator: std.mem.Allocator,
    map: maplibre.Map,
    vulkan: maplibre.VulkanContextDescriptor,
    images: [2]u64,
    views: [2]u64,
    new_images: [2]u64,
    new_views: [2]u64,
) !void {
    // #region attach-options
    const options = maplibre.RenderSessionAttachOptions{
        .driver = .core_worker,
        .frame_wake = .{ .callback = onFrameWake },
        // Only if your renderer also submits to the graphics queue.
        .queue_lock = .{ .lock = lockQueue, .unlock = unlockQueue },
    };
    // #endregion attach-options

    // #region attach-borrowed
    // Your VkImage and VkImageView for each texture of the ring.
    const textures = [_]maplibre.VulkanBorrowedTexture{
        .{ .image = images[0], .image_view = views[0] },
        .{ .image = images[1], .image_view = views[1] },
    };
    const ring = maplibre.VulkanBorrowedTextureDescriptor{
        // The map's size in logical pixels.
        .logical_size = .{ .width = 800, .height = 600 },
        // The size of your textures in device pixels.
        .physical_width = 1600,
        .physical_height = 1200,
        .context = vulkan,
        .textures = &textures,
        .format = 37, // VK_FORMAT_R8G8B8A8_UNORM
        .final_layout = 5, // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    const borrowed = try map.attachVulkanBorrowedTexture(
        allocator,
        ring,
        options,
        null,
    );
    session = borrowed.session;
    try borrowed.ready.await(null);
    // #endregion attach-borrowed

    try drawFrame();

    // #region resize-borrowed
    // Your new textures, at the new size.
    const new_textures = [_]maplibre.VulkanBorrowedTexture{
        .{ .image = new_images[0], .image_view = new_views[0] },
        .{ .image = new_images[1], .image_view = new_views[1] },
    };
    const resized = maplibre.VulkanBorrowedTextureReplacement{
        .logical_size = .{ .width = 1024, .height = 768 },
        .physical_width = 2048,
        .physical_height = 1536,
        .textures = &new_textures,
    };

    const replaced = try session.setVulkanBorrowedTextureTarget(
        allocator,
        resized,
        null,
    );
    try replaced.await(null);

    // vkDestroyImageView and vkDestroyImage
    destroyTextures(images, views);
    // #endregion resize-borrowed
}
