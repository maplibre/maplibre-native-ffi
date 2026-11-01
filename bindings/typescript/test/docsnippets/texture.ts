// Snippets for the docs site's Render to a texture page. They are type-checked
// with the tests, which never run them.

import {
  type MapHandle,
  QueueLock,
  RenderDriverKind,
  type RenderSessionAttachOptions,
  type RenderSessionHandle,
  type VulkanBorrowedTextureDescriptor,
  type VulkanContextDescriptor,
  type VulkanOwnedTextureDescriptor,
} from "../../src/index.ts";

let session: RenderSessionHandle;

// The frame wake from the Render to a window page.
function onFrameWake(): void {}

// The host's draw, which samples the image view.
function drawMap(_imageView: bigint): void {}

type Pair = readonly [bigint, bigint];

function destroyTextures(_images: Pair, _views: Pair): void {}

export function drawFrame(): void {
  // #region acquire
  const frame = session.acquireFrame();
  if (frame === undefined) {
    return; // No new frame yet.
  }

  // Sample the texture, and wait for those reads to finish.
  frame.withVulkanTexture((texture) => drawMap(texture.imageView));

  frame.release();
  // #endregion acquire
}

export async function renderToTexture(
  map: MapHandle,
  vulkan: VulkanContextDescriptor,
  lockQueue: bigint,
  unlockQueue: bigint,
  images: Pair,
  views: Pair,
  newImages: Pair,
  newViews: Pair,
): Promise<void> {
  // #region attach-owned
  const texture: VulkanOwnedTextureDescriptor = {
    // The texture's size in logical pixels, and its scale factor.
    extent: { width: 800, height: 600, scaleFactor: 2 },
    // Your VkInstance, VkPhysicalDevice, VkDevice, and graphics VkQueue.
    context: vulkan,
  };
  const options: RenderSessionAttachOptions = {
    driver: RenderDriverKind.coreWorker,
    frameWake: onFrameWake,
    // Only if your renderer also submits to the graphics queue. The lock and
    // unlock are native function pointers.
    queueLock: QueueLock.native({ lock: lockQueue, unlock: unlockQueue }),
  };

  const owned = map.attachVulkanOwnedTexture(texture, options);
  session = owned.session;
  await owned.ready;
  // #endregion attach-owned

  drawFrame();
  await session.close();

  // #region attach-borrowed
  let ring: VulkanBorrowedTextureDescriptor = {
    extent: { width: 800, height: 600, scaleFactor: 2 },
    // The size of your textures in device pixels.
    physicalWidth: 1600,
    physicalHeight: 1200,
    context: vulkan,
    // Your VkImage and VkImageView for each texture of the ring.
    textures: [
      { image: images[0], imageView: views[0] },
      { image: images[1], imageView: views[1] },
    ],
    format: 37, // VK_FORMAT_R8G8B8A8_UNORM
    finalLayout: 5, // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  };

  // The same attach options as for an owned ring. A queue lock serves one
  // attach, so pass a new one.
  const borrowed = map.attachVulkanBorrowedTexture(ring, {
    ...options,
    queueLock: QueueLock.native({ lock: lockQueue, unlock: unlockQueue }),
  });
  session = borrowed.session;
  await borrowed.ready;
  // #endregion attach-borrowed

  // #region resize-borrowed
  ring = {
    ...ring,
    extent: { width: 1024, height: 768, scaleFactor: 2 },
    physicalWidth: 2048,
    physicalHeight: 1536,
    // Your new textures, at the new size.
    textures: [
      { image: newImages[0], imageView: newViews[0] },
      { image: newImages[1], imageView: newViews[1] },
    ],
  };

  await session.setVulkanBorrowedTextureTarget(ring);
  destroyTextures(images, views); // vkDestroyImageView and vkDestroyImage
  // #endregion resize-borrowed
}
