// Snippets for the docs site's Render to a texture page. They compile
// with the tests, which never run them, since they need your Vulkan
// device and textures.
package org.maplibre.nativeffi.docsnippets;

import java.util.List;
import java.util.concurrent.locks.Lock;
import org.maplibre.nativeffi.AcquiredFrameHandle;
import org.maplibre.nativeffi.GpuDeviceHandle;
import org.maplibre.nativeffi.GpuSync;
import org.maplibre.nativeffi.GpuSyncKind;
import org.maplibre.nativeffi.LogicalSize;
import org.maplibre.nativeffi.MapHandle;
import org.maplibre.nativeffi.MaplibreNative;
import org.maplibre.nativeffi.QueueLock;
import org.maplibre.nativeffi.RenderDriverKind;
import org.maplibre.nativeffi.RenderSessionAttachOptions;
import org.maplibre.nativeffi.RenderSessionHandle;
import org.maplibre.nativeffi.RenderTarget;
import org.maplibre.nativeffi.VulkanBorrowedTexture;
import org.maplibre.nativeffi.VulkanBorrowedTexturesTarget;
import org.maplibre.nativeffi.VulkanContextDescriptor;
import org.maplibre.nativeffi.VulkanDeviceDescriptor;
import org.maplibre.nativeffi.Wake;

public class TextureTest {
  private RenderSessionHandle session;

  // The frame wake from the Render to a window page.
  private void onFrameWake() {}

  // The host's draw, which samples the image view.
  private void drawMap(long imageView) {}

  private void destroyTextures(List<Long> images, List<Long> views) {}

  private void drawFrame() {
    // #region acquire
    AcquiredFrameHandle frame = session.acquireFrame();
    if (frame == null) return; // No new frame yet.

    // Sample the texture, and wait for those reads to finish.
    MaplibreNative.withVulkanTexture(
        frame,
        texture -> {
          drawMap(texture.getImageView());
          return null;
        });

    frame.release(GpuSync.of().withKind(GpuSyncKind.CPU_COMPLETE));
    // #endregion acquire
  }

  // queueMutex is the lock your renderer takes around its own calls
  // on the graphics queue.
  private void renderToTexture(
      MapHandle map,
      VulkanContextDescriptor vulkan,
      Lock queueMutex,
      List<Long> images,
      List<Long> views,
      List<Long> newImages,
      List<Long> newViews) {
    // #region attach-options
    RenderSessionAttachOptions options =
        RenderSessionAttachOptions.of(RenderDriverKind.CORE_WORKER)
            .withFrameWake(new Wake(this::onFrameWake));
    // #endregion attach-options

    // #region attach-borrowed
    // Your VkDevice and graphics queue.
    GpuDeviceHandle device =
        MaplibreNative.wrapVulkan(
            GpuDeviceHandle.Companion,
            VulkanDeviceDescriptor.of(vulkan)
                // Only if your renderer also submits to the graphics
                // queue.
                .withQueueLock(
                    new QueueLock(
                        queueMutex::lock, queueMutex::unlock))
                // The device extensions that you enabled on your
                // VkDevice.
                .withEnabledExtensions(
                    List.of(
                        "VK_KHR_swapchain",
                        "VK_KHR_external_semaphore_fd")));

    VulkanBorrowedTexturesTarget ring =
        VulkanBorrowedTexturesTarget.of(
            // The map's size in logical pixels.
            LogicalSize.of(800, 600),
            // The size of your textures in device pixels.
            1600,
            1200,
            // Your VkImage and VkImageView for each texture of the
            // ring.
            List.of(
                VulkanBorrowedTexture.of(images.get(0), views.get(0)),
                VulkanBorrowedTexture.of(
                    images.get(1), views.get(1))),
            37, // VK_FORMAT_R8G8B8A8_UNORM
            5); // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL

    session =
        map.attach(
            new RenderTarget.VulkanBorrowedTextures(ring),
            device,
            options);
    // The session keeps the device until it detaches.
    device.release();
    session.getReady().join();
    // #endregion attach-borrowed

    drawFrame();

    // #region resize-borrowed
    VulkanBorrowedTexturesTarget resized =
        VulkanBorrowedTexturesTarget.of(
            LogicalSize.of(1024, 768),
            2048,
            1536,
            // Your new textures, at the new size.
            List.of(
                VulkanBorrowedTexture.of(
                    newImages.get(0), newViews.get(0)),
                VulkanBorrowedTexture.of(
                    newImages.get(1), newViews.get(1))),
            37,
            5);

    session
        .setTarget(new RenderTarget.VulkanBorrowedTextures(resized))
        .join();
    // vkDestroyImageView and vkDestroyImage
    destroyTextures(images, views);
    // #endregion resize-borrowed
  }
}
