// Snippets for the docs site's Render to a texture page. They compile with the tests, which never
// run them.
package org.maplibre.nativeffi.docsnippets

import java.util.concurrent.locks.Lock
import org.maplibre.nativeffi.LogicalExtent
import org.maplibre.nativeffi.MapHandle
import org.maplibre.nativeffi.QueueLock
import org.maplibre.nativeffi.RenderDriverKind
import org.maplibre.nativeffi.RenderSessionAttachOptions
import org.maplibre.nativeffi.RenderSessionHandle
import org.maplibre.nativeffi.VulkanBorrowedTexture
import org.maplibre.nativeffi.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.VulkanContextDescriptor
import org.maplibre.nativeffi.VulkanOwnedTextureDescriptor
import org.maplibre.nativeffi.Wake
import org.maplibre.nativeffi.attachVulkanBorrowedTexture
import org.maplibre.nativeffi.attachVulkanOwnedTexture
import org.maplibre.nativeffi.setVulkanBorrowedTextureTarget
import org.maplibre.nativeffi.withVulkanTexture

private lateinit var session: RenderSessionHandle

// The frame wake from the Render to a window page.
private fun onFrameWake() {}

// The host's draw, which samples the image view.
private fun drawMap(imageView: ULong) {}

private fun destroyTextures(images: List<ULong>, views: List<ULong>) {}

private fun drawFrame() {
  // #region acquire
  val frame = session.acquireFrame() ?: return // No new frame yet.

  // Sample the texture, and wait for those reads to finish.
  frame.withVulkanTexture { texture -> drawMap(texture.imageView) }

  frame.release(consumerCompletion = null)
  // #endregion acquire
}

// queueMutex is the lock your renderer takes around its own calls on the graphics queue.
private suspend fun renderToTexture(
  map: MapHandle,
  vulkan: VulkanContextDescriptor,
  queueMutex: Lock,
  images: List<ULong>,
  views: List<ULong>,
  newImages: List<ULong>,
  newViews: List<ULong>,
) {
  // #region attach-owned
  val texture =
    VulkanOwnedTextureDescriptor(
      // The texture's size in logical pixels, and its scale factor.
      extent = LogicalExtent(width = 800u, height = 600u, scaleFactor = 2.0),
      // Your VkInstance, VkPhysicalDevice, VkDevice, and graphics VkQueue.
      context = vulkan,
    )
  val options =
    RenderSessionAttachOptions(
      driver = RenderDriverKind.CORE_WORKER,
      frameWake = Wake(::onFrameWake),
      // Only if your renderer also submits to the graphics queue.
      queueLock = QueueLock(lock = { queueMutex.lock() }, unlock = { queueMutex.unlock() }),
    )

  val owned = map.attachVulkanOwnedTexture(texture, options)
  session = owned.session
  owned.ready.await()
  // #endregion attach-owned

  drawFrame()
  session.release().await()

  // #region attach-borrowed
  var ring =
    VulkanBorrowedTextureDescriptor(
      extent = LogicalExtent(width = 800u, height = 600u, scaleFactor = 2.0),
      // The size of your textures in device pixels.
      physicalWidth = 1600u,
      physicalHeight = 1200u,
      context = vulkan,
      // Your VkImage and VkImageView for each texture of the ring.
      textures =
        listOf(
          VulkanBorrowedTexture(image = images[0], imageView = views[0]),
          VulkanBorrowedTexture(image = images[1], imageView = views[1]),
        ),
      format = 37u, // VK_FORMAT_R8G8B8A8_UNORM
      finalLayout = 5u, // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    )

  // The same attach options as for an owned ring.
  val borrowed = map.attachVulkanBorrowedTexture(ring, options)
  session = borrowed.session
  borrowed.ready.await()
  // #endregion attach-borrowed

  // #region resize-borrowed
  ring =
    ring.copy(
      extent = LogicalExtent(width = 1024u, height = 768u, scaleFactor = 2.0),
      physicalWidth = 2048u,
      physicalHeight = 1536u,
      // Your new textures, at the new size.
      textures =
        listOf(
          VulkanBorrowedTexture(image = newImages[0], imageView = newViews[0]),
          VulkanBorrowedTexture(image = newImages[1], imageView = newViews[1]),
        ),
    )

  session.setVulkanBorrowedTextureTarget(ring).await()
  destroyTextures(images, views) // vkDestroyImageView and vkDestroyImage
  // #endregion resize-borrowed
}
