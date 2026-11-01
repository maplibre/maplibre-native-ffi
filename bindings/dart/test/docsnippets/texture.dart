// Snippets for the docs site's Render to a texture page. They are analyzed
// with the tests, which never run them.

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

late RenderSessionHandle session;

// The frame wake from the Render to a window page.
void onFrameWake() {}

// The host's draw, which samples the image view.
void drawMap(int imageView) {}

void destroyTextures(List<int> images, List<int> views) {}

void drawFrame() {
  // #region acquire
  final frame = session.acquireFrame();
  if (frame == null) {
    return; // No new frame yet.
  }

  // Sample the texture, and wait for those reads to finish.
  frame.withVulkanTexture((texture) => drawMap(texture.imageView));

  frame.release();
  // #endregion acquire
}

Future<void> renderToTexture(
  MapHandle map,
  VulkanContextDescriptor vulkan,
  NativePointer lockQueue,
  NativePointer unlockQueue,
  List<int> images,
  List<int> views,
  List<int> newImages,
  List<int> newViews,
) async {
  // #region attach-owned
  final texture = VulkanOwnedTextureDescriptor(
    // The texture's size in logical pixels, and its scale factor.
    extent: const LogicalExtent(width: 800, height: 600, scaleFactor: 2),
    // Your VkInstance, VkPhysicalDevice, VkDevice, and graphics VkQueue.
    context: vulkan,
  );
  final options = RenderSessionAttachOptions(
    driver: RenderDriverKind.coreWorker,
    frameWake: const Wake(callback: onFrameWake),
    // Only if your renderer also submits to the graphics queue. The lock and
    // unlock are native function pointers.
    queueLock: QueueLock.native(lock: lockQueue, unlock: unlockQueue),
  );

  final owned = map.attachVulkanOwnedTexture(texture, options);
  session = owned.session;
  await owned.ready;
  // #endregion attach-owned

  drawFrame();
  await session.close();

  // #region attach-borrowed
  var ring = VulkanBorrowedTextureDescriptor(
    extent: const LogicalExtent(width: 800, height: 600, scaleFactor: 2),
    // The size of your textures in device pixels.
    physicalWidth: 1600,
    physicalHeight: 1200,
    context: vulkan,
    // Your VkImage and VkImageView for each texture of the ring.
    textures: [
      VulkanBorrowedTexture(image: images[0], imageView: views[0]),
      VulkanBorrowedTexture(image: images[1], imageView: views[1]),
    ],
    format: 37, // VK_FORMAT_R8G8B8A8_UNORM
    finalLayout: 5, // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  );

  // The same attach options as for an owned ring. A queue lock serves one
  // attach, so pass a new one.
  final borrowed = map.attachVulkanBorrowedTexture(
    ring,
    options.copyWith(
      queueLock: QueueLock.native(lock: lockQueue, unlock: unlockQueue),
    ),
  );
  session = borrowed.session;
  await borrowed.ready;
  // #endregion attach-borrowed

  // #region resize-borrowed
  ring = ring.copyWith(
    extent: const LogicalExtent(width: 1024, height: 768, scaleFactor: 2),
    physicalWidth: 2048,
    physicalHeight: 1536,
    // Your new textures, at the new size.
    textures: [
      VulkanBorrowedTexture(image: newImages[0], imageView: newViews[0]),
      VulkanBorrowedTexture(image: newImages[1], imageView: newViews[1]),
    ],
  );

  await session.setVulkanBorrowedTextureTarget(ring);
  destroyTextures(images, views); // vkDestroyImageView and vkDestroyImage
  // #endregion resize-borrowed
}
