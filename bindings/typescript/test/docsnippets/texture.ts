// Snippets for the docs site's Render to a texture page. They are type-checked
// with the tests, which never run them.

import {
  GpuDeviceHandle,
  GpuSyncKind,
  type MapHandle,
  QueueLock,
  RenderDriverKind,
  type RenderSessionAttachOptions,
  type RenderSessionHandle,
  type RenderTarget,
  type VulkanContextDescriptor,
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

  frame.release({
    kind: GpuSyncKind.cpuComplete,
    handle: 0n,
    value: 0n,
  });
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
  // #region attach-options
  const options: RenderSessionAttachOptions = {
    driver: RenderDriverKind.coreWorker,
    frameWake: { callback: onFrameWake },
  };
  // #endregion attach-options

  // #region attach-borrowed
  // Your VkDevice and graphics queue.
  const device = GpuDeviceHandle.wrapVulkan({
    context: vulkan,
    // Only if your renderer also submits to the graphics queue. The
    // lock and unlock are native function pointers.
    queueLock: QueueLock.native({
      lock: lockQueue,
      unlock: unlockQueue,
    }),
    // The device extensions that you enabled on your VkDevice.
    enabledExtensions: [
      "VK_KHR_swapchain",
      "VK_KHR_external_semaphore_fd",
    ],
  });

  const ring: RenderTarget = {
    kind: "vulkanBorrowedTextures",
    value: {
      // The map's size in logical pixels.
      logicalSize: { width: 800, height: 600 },
      // The size of your textures in device pixels.
      physicalWidth: 1600,
      physicalHeight: 1200,
      // Your VkImage and VkImageView for each texture of the ring.
      textures: [
        { image: images[0], imageView: views[0] },
        { image: images[1], imageView: views[1] },
      ],
      format: 37, // VK_FORMAT_R8G8B8A8_UNORM
      finalLayout: 5, // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    },
  };

  session = map.attach(ring, device, options);
  // The session keeps the device until it detaches.
  device.release();
  await session.ready;
  // #endregion attach-borrowed

  // #region resize-borrowed
  const resized: RenderTarget = {
    kind: "vulkanBorrowedTextures",
    value: {
      logicalSize: { width: 1024, height: 768 },
      physicalWidth: 2048,
      physicalHeight: 1536,
      // Your new textures, at the new size.
      textures: [
        { image: newImages[0], imageView: newViews[0] },
        { image: newImages[1], imageView: newViews[1] },
      ],
      format: 37,
      finalLayout: 5,
    },
  };

  await session.setTarget(resized);
  // vkDestroyImageView and vkDestroyImage
  destroyTextures(images, views);
  // #endregion resize-borrowed
}
