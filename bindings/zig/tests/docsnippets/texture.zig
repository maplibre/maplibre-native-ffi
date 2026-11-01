//! Snippets for the docs site's Render to a texture page. They compile with the
//! tests, which never run them.

const std = @import("std");
const maplibre = @import("maplibre_native_ffi");

comptime {
    _ = &renderToTexture;
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
    var frame = (try session.acquireFrame(null)) orelse return; // No new frame yet.

    // Sample the texture, and wait for those reads to finish.
    try frame.getVulkanTexture({}, drawMap, null);

    try frame.release(null, null);
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
    // #region attach-owned
    const texture = maplibre.VulkanOwnedTextureDescriptor{
        // The texture's size in logical pixels, and its scale factor.
        .extent = .{ .width = 800, .height = 600, .scale_factor = 2.0 },
        // Your VkInstance, VkPhysicalDevice, VkDevice, and graphics VkQueue.
        .context = vulkan,
    };
    const options = maplibre.RenderSessionAttachOptions{
        .driver = .core_worker,
        .frame_wake = .{ .callback = onFrameWake },
        // Only if your renderer also submits to the graphics queue.
        .queue_lock = .{ .lock = lockQueue, .unlock = unlockQueue },
    };

    var owned = try map.attachVulkanOwnedTexture(texture, options, null);
    defer owned.ready.deinit();
    session = owned.session;
    try owned.ready.wait(null);
    // #endregion attach-owned

    try drawFrame();
    var closed = try session.release(null);
    defer closed.deinit();
    try closed.wait(null);

    // #region attach-borrowed
    // Your VkImage and VkImageView for each texture of the ring.
    const textures = [_]maplibre.VulkanBorrowedTexture{
        .{ .image = images[0], .image_view = views[0] },
        .{ .image = images[1], .image_view = views[1] },
    };
    var ring = maplibre.VulkanBorrowedTextureDescriptor{
        .extent = .{ .width = 800, .height = 600, .scale_factor = 2.0 },
        // The size of your textures in device pixels.
        .physical_width = 1600,
        .physical_height = 1200,
        .context = vulkan,
        .textures = &textures,
        .format = 37, // VK_FORMAT_R8G8B8A8_UNORM
        .final_layout = 5, // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    // The same attach options as for an owned ring.
    var borrowed = try map.attachVulkanBorrowedTexture(allocator, ring, options, null);
    defer borrowed.ready.deinit();
    session = borrowed.session;
    try borrowed.ready.wait(null);
    // #endregion attach-borrowed

    // #region resize-borrowed
    // Your new textures, at the new size.
    const new_textures = [_]maplibre.VulkanBorrowedTexture{
        .{ .image = new_images[0], .image_view = new_views[0] },
        .{ .image = new_images[1], .image_view = new_views[1] },
    };
    ring.extent = .{ .width = 1024, .height = 768, .scale_factor = 2.0 };
    ring.physical_width = 2048;
    ring.physical_height = 1536;
    ring.textures = &new_textures;

    var replaced = try session.setVulkanBorrowedTextureTarget(allocator, ring, null);
    defer replaced.deinit();
    try replaced.wait(null);

    destroyTextures(images, views); // vkDestroyImageView and vkDestroyImage
    // #endregion resize-borrowed
}
