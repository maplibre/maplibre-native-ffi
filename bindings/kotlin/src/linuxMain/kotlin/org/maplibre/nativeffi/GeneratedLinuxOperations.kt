// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * Gets the dmabuf of an exported ring's acquired frame.
 *
 * Wait for `AcquiredFrameHandle.withProducerSync()` before you read the buffer. Once you release or
 * discard the frame, the session may render into the buffer again. You can map a buffer with
 * DRM_FORMAT_MOD_LINEAR, which has one plane, with `mmap`, and bracket your reads with
 * DMA_BUF_IOCTL_SYNC.
 *
 * The session owns the planes' file descriptors, so don't close them. To keep the buffer after you
 * release the frame, duplicate each file descriptor with `dup`. A duplicate keeps the buffer's
 * memory, but not its contents.
 *
 * See `mln_acquired_frame_get_dma_buf` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` when the session's ring shares its textures in another way.
 * - `Status.TARGET_LOST` when the session failed, or you detached, abandoned, or disposed it.
 */
public fun <T> AcquiredFrameHandle.withDmaBuf(block: (DmaBufFrame) -> T): T =
  nativeCall(this, binding) {
    borrowView(
      { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
      { C.mln_acquired_frame_view_end(it) },
    ) { scope ->
      val out = sized(w(48, 56, 44), w(8, 8, 4))
      check(C.mln_acquired_frame_get_dma_buf(handle, out, diagnostic))
      block(readDmaBufFrame(out, scope))
    }
  }

/**
 * Gets the memory of an exported ring's acquired frame, as an opaque file descriptor.
 *
 * Wait for `AcquiredFrameHandle.withProducerSync()` before you read the image. Once you release or
 * discard the frame, the session may render into the image again.
 *
 * The session owns the file descriptor, so don't close it. An import takes the descriptor, so
 * import a duplicate from `dup`. Memory that you import keeps the image's memory, but not its
 * contents.
 *
 * See `mln_acquired_frame_get_opaque_fd` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` when the session's ring shares its textures in another way.
 * - `Status.TARGET_LOST` when the session failed, or you detached, abandoned, or disposed it.
 */
public fun <T> AcquiredFrameHandle.withOpaqueFd(block: (OpaqueFdFrame) -> T): T =
  nativeCall(this, binding) {
    borrowView(
      { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
      { C.mln_acquired_frame_view_end(it) },
    ) { scope ->
      val out = sized(w(72, 72, 64), w(8, 8, 4))
      check(C.mln_acquired_frame_get_opaque_fd(handle, out, diagnostic))
      block(readOpaqueFdFrame(out, scope))
    }
  }

/**
 * Gets the Vulkan image of an acquired frame.
 *
 * It gets a borrowed ring's image, or an exported ring's on a Vulkan device: the image whose memory
 * the ring shares, and the frame's depth image with `RenderSessionAttachOptions.keepDepth`. An
 * exported ring's frame releases them to VK_QUEUE_FAMILY_EXTERNAL, so acquire them from there. Wait
 * for `AcquiredFrameHandle.withProducerSync()` before you use them.
 *
 * See `mln_acquired_frame_get_vulkan_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 *
 * Errors:
 *
 * - `Status.TARGET_LOST` when the session failed, or you detached, abandoned, or disposed it.
 */
public fun <T> AcquiredFrameHandle.withVulkanTexture(block: (VulkanTextureFrame) -> T): T =
  nativeCall(this, binding) {
    borrowView(
      { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
      { C.mln_acquired_frame_view_end(it) },
    ) { scope ->
      val out = sized(w(88, 88, 76), w(8, 8, 4))
      check(C.mln_acquired_frame_get_vulkan_texture(handle, out, diagnostic))
      block(readVulkanTextureFrame(out, scope))
    }
  }

/**
 * Wraps your Vulkan device and queue, so that sessions render on them.
 *
 * Sessions on the device render into your images, present to your surfaces, and submit to your
 * queue, under your queue lock when you give one. Sessions of several devices that wrap one queue
 * never submit to it at once.
 *
 * See `mln_gpu_device_wrap_vulkan` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` when `supportedRenderBackendMask()` lacks `RenderBackendFlag.VULKAN`.
 * - `Status.NATIVE_ERROR` when the library can't load the device's functions.
 */
public fun GpuDeviceHandle.Companion.wrapVulkan(
  descriptor: VulkanDeviceDescriptor,
): GpuDeviceHandle =
  nativeCall(null, null) {
    val out = allocate(8, w(8, 8, 4))
    check(C.mln_gpu_device_wrap_vulkan(writeVulkanDeviceDescriptor(descriptor), out, diagnostic))
    adoptRegistered(
      out,
      GeneratedOwnerDisposal::gpuDevice,
      { GpuDeviceHandle.wrap(it) },
      { it.bindingCallbacks },
      { it.release() },
    )
  }

/**
 * Gets the Vulkan instance, device, and graphics queue of a Vulkan device.
 *
 * Use them to make the images of a borrowed ring, or a surface, that sessions on the device render
 * to. The sessions on the device share its graphics queue. To submit to the queue yourself, create
 * the device with `GpuDeviceOptions.queueLock`.
 *
 * See `mln_gpu_device_get_vulkan_context` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` when the device doesn't render with Vulkan.
 */
public fun GpuDeviceHandle.getVulkanContext(): GpuDeviceVulkanContextResult =
  nativeCall(this, binding) {
    val out = sized(w(32, 64), w(4, 8))
    check(C.mln_gpu_device_get_vulkan_context(handle, out, diagnostic))
    readGpuDeviceVulkanContextResult(out)
  }
