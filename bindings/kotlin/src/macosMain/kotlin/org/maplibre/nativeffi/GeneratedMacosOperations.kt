// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

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
