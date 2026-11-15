// Snippets for the docs site's Render to a texture page. They compile
// with the tests, which never
// run them.
package org.maplibre.nativeffi.docsnippets

import java.util.concurrent.locks.Lock
import org.maplibre.nativeffi.GpuDeviceHandle
import org.maplibre.nativeffi.GpuSync
import org.maplibre.nativeffi.GpuSyncKind
import org.maplibre.nativeffi.LogicalSize
import org.maplibre.nativeffi.MapHandle
import org.maplibre.nativeffi.QueueLock
import org.maplibre.nativeffi.RenderDriverKind
import org.maplibre.nativeffi.RenderSessionAttachOptions
import org.maplibre.nativeffi.RenderSessionHandle
import org.maplibre.nativeffi.RenderTarget
import org.maplibre.nativeffi.VulkanBorrowedTexture
import org.maplibre.nativeffi.VulkanBorrowedTexturesTarget
import org.maplibre.nativeffi.VulkanContextDescriptor
import org.maplibre.nativeffi.VulkanDeviceDescriptor
import org.maplibre.nativeffi.Wake
import org.maplibre.nativeffi.withVulkanTexture
import org.maplibre.nativeffi.wrapVulkan

private lateinit var session: RenderSessionHandle

// The frame wake from the Render to a window page.
private fun onFrameWake() {}

// The host's draw, which samples the image view.
private fun drawMap(imageView: ULong) {}

private fun destroyTextures(
  images: List<ULong>,
  views: List<ULong>,
) {}

private fun drawFrame() {
  // #region acquire
  val frame = session.acquireFrame() ?: return // No new frame yet.

  // Sample the texture, and wait for those reads to finish.
  frame.withVulkanTexture { texture -> drawMap(texture.imageView) }

  frame.release(GpuSync(kind = GpuSyncKind.CPU_COMPLETE))
  // #endregion acquire
}

// queueMutex is the lock your renderer takes around its own calls on
// the graphics queue.
private suspend fun renderToTexture(
  map: MapHandle,
  vulkan: VulkanContextDescriptor,
  queueMutex: Lock,
  images: List<ULong>,
  views: List<ULong>,
  newImages: List<ULong>,
  newViews: List<ULong>,
) {
  // #region attach-options
  val options =
    RenderSessionAttachOptions(
      driver = RenderDriverKind.CORE_WORKER,
      frameWake = Wake(::onFrameWake),
    )
  // #endregion attach-options

  // #region attach-borrowed
  // Your VkDevice and graphics queue.
  val device =
    GpuDeviceHandle.wrapVulkan(
      VulkanDeviceDescriptor(
        context = vulkan,
        // Only if your renderer also submits to the graphics queue.
        queueLock =
          QueueLock(
            lock = { queueMutex.lock() },
            unlock = { queueMutex.unlock() },
          ),
        // The device extensions that you enabled on your VkDevice.
        enabledExtensions =
          listOf(
            "VK_KHR_swapchain",
            "VK_KHR_external_semaphore_fd",
          ),
      ),
    )

  val ring =
    VulkanBorrowedTexturesTarget(
      // The map's size in logical pixels.
      logicalSize = LogicalSize(width = 800u, height = 600u),
      // The size of your textures in device pixels.
      physicalWidth = 1600u,
      physicalHeight = 1200u,
      // Your VkImage and VkImageView for each texture of the ring.
      textures =
        listOf(
          VulkanBorrowedTexture(
            image = images[0],
            imageView = views[0],
          ),
          VulkanBorrowedTexture(
            image = images[1],
            imageView = views[1],
          ),
        ),
      format = 37u, // VK_FORMAT_R8G8B8A8_UNORM
      finalLayout = 5u, // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    )

  session =
    map.attach(
      RenderTarget.VulkanBorrowedTextures(ring),
      device,
      options,
    )
  // The session keeps the device until it detaches.
  device.release()
  session.ready.await()
  // #endregion attach-borrowed

  drawFrame()

  // #region resize-borrowed
  val resized =
    ring.copy(
      logicalSize = LogicalSize(width = 1024u, height = 768u),
      physicalWidth = 2048u,
      physicalHeight = 1536u,
      // Your new textures, at the new size.
      textures =
        listOf(
          VulkanBorrowedTexture(
            image = newImages[0],
            imageView = newViews[0],
          ),
          VulkanBorrowedTexture(
            image = newImages[1],
            imageView = newViews[1],
          ),
        ),
    )

  session
    .setTarget(RenderTarget.VulkanBorrowedTextures(resized))
    .await()
  // vkDestroyImageView and vkDestroyImage
  destroyTextures(images, views)
  // #endregion resize-borrowed
}
