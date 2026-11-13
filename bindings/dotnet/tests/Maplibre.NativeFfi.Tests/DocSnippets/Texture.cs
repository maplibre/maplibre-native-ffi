// Snippets for the docs site's Render to a texture page. They compile with the
// tests, which never run them.

using System.Runtime.Versioning;

namespace Maplibre.NativeFfi.Tests.DocSnippets;

[SupportedOSPlatform("android")]
[SupportedOSPlatform("linux")]
[SupportedOSPlatform("macos")]
[SupportedOSPlatform("windows")]
internal static class Texture
{
    private static RenderSessionHandle session = null!;

    // The frame wake from the Render to a window page.
    private static void OnFrameWake() { }

    // Stand-ins for the host's mutex on its graphics queue.
    private static void LockQueue() { }

    private static void UnlockQueue() { }

    // The host's draw, which samples the image view.
    private static void DrawMap(ulong imageView) { }

    private static void DestroyTextures(
        ulong[] images,
        ulong[] views
    ) { }

    private static void DrawFrame()
    {
        // #region acquire
        var frame = session.AcquireFrame();
        if (frame is null)
            return; // No new frame yet.

        // Sample the texture, and wait for those reads to finish.
        frame.WithVulkanTexture(texture =>
            DrawMap(texture.ImageView)
        );

        frame.Release(new GpuSync { Kind = GpuSyncKind.CpuComplete });
        // #endregion acquire
    }

    internal static async Task Run(
        MapHandle map,
        VulkanContextDescriptor vulkan,
        ulong[] images,
        ulong[] views,
        ulong[] newImages,
        ulong[] newViews
    )
    {
        // #region attach-options
        var options = new RenderSessionAttachOptions
        {
            Driver = RenderDriverKind.CoreWorker,
            FrameWake = new Wake(OnFrameWake),
        };
        // #endregion attach-options

        // #region attach-borrowed
        // Your VkDevice and graphics queue.
        using var device = GpuDeviceHandle.WrapVulkan(
            new VulkanDeviceDescriptor
            {
                Context = vulkan,
                // Only if your renderer also
                // submits to the graphics queue.
                QueueLock = new QueueLock(LockQueue, UnlockQueue),
                // The device extensions that you
                // enabled on your VkDevice.
                EnabledExtensions =
                [
                    "VK_KHR_swapchain",
                    "VK_KHR_external_semaphore_fd",
                ],
            }
        );

        var ring = new RenderTarget.VulkanBorrowedTextures(
            new VulkanBorrowedTexturesTarget
            {
                // The map's size in logical pixels.
                LogicalSize = new LogicalSize(800, 600),
                // The size of your textures in device pixels.
                PhysicalWidth = 1600,
                PhysicalHeight = 1200,
                // Your VkImage and VkImageView
                // for each texture of the ring.
                Textures =
                [
                    new VulkanBorrowedTexture(images[0], views[0]),
                    new VulkanBorrowedTexture(images[1], views[1]),
                ],
                // VK_FORMAT_R8G8B8A8_UNORM
                Format = 37,
                // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                FinalLayout = 5,
            }
        );

        // The session keeps the device until it detaches.
        session = map.Attach(ring, device, options);
        await session.Ready;
        // #endregion attach-borrowed

        // #region resize-borrowed
        var resized = new RenderTarget.VulkanBorrowedTextures(
            new VulkanBorrowedTexturesTarget
            {
                LogicalSize = new LogicalSize(1024, 768),
                PhysicalWidth = 2048,
                PhysicalHeight = 1536,
                // Your new textures, at the new size.
                Textures =
                [
                    new VulkanBorrowedTexture(newImages[0], newViews[0]),
                    new VulkanBorrowedTexture(newImages[1], newViews[1]),
                ],
                Format = 37,
                FinalLayout = 5,
            }
        );

        await session.SetTargetAsync(resized);
        // vkDestroyImageView and vkDestroyImage
        DestroyTextures(images, views);
        // #endregion resize-borrowed
    }
}
