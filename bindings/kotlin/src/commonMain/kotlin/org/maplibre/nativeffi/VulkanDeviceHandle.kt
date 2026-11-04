// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * A Vulkan instance, device, and queue that the library creates and owns.
 *
 * See `mln_vulkan_device` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class VulkanDeviceHandle
internal constructor(
  handle: Long,
) : AutoCloseable {
  internal val binding =
    HandleState("VulkanDeviceHandle", handle, GeneratedOwnerDisposal::vulkanDevice)
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /** The native handle value. It stays readable after release. */
  public val id: ULong
    get() = binding.handle.toULong()

  /** Whether a release, dispose, or discard has ended the handle. */
  public val isClosed: Boolean
    get() = binding.isReleased

  /** Calls [release], for the standard library's `use`. */
  public override fun close() {
    release()
  }

  /**
   * Releases a Vulkan device.
   *
   * Render sessions that attached with the device's context keep using it until they detach or
   * retire, and the library destroys the device after the last of them.
   *
   * See `mln_vulkan_device_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html).
   */
  public fun release(): Unit = nativeClose(this, binding) { C.mln_vulkan_device_release(handle) }

  override fun toString(): String =
    "VulkanDeviceHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"

  public companion object {}
}
