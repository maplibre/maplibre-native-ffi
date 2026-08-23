package org.maplibre.nativeffi.examples.androidmap

import android.view.Surface
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.VulkanContextDescriptor
import org.maplibre.nativeffi.render.VulkanHandle

/**
 * The Vulkan context. The instance, the device, and the queue are chosen for the `VkSurfaceKHR`
 * this context created from the host window, and Vulkan destroys every swapchain before its
 * surface, so a window the platform takes away takes this context and its session with it.
 */
internal class VulkanGraphicsContext private constructor(private var handle: Long) :
  GraphicsContext {
  override val backendName: String = "vulkan"

  override val hasSurface: Boolean
    get() = handle != 0L

  val descriptor: VulkanContextDescriptor
    get() =
      VulkanContextDescriptor(
        NativePointer.ofAddress(VulkanNativeBridge.instance(handle)),
        NativePointer.ofAddress(VulkanNativeBridge.physicalDevice(handle)),
        NativePointer.ofAddress(VulkanNativeBridge.device(handle)),
        NativePointer.ofAddress(VulkanNativeBridge.graphicsQueue(handle)),
        VulkanNativeBridge.graphicsQueueFamilyIndex(handle),
        NativePointer.ofAddress(VulkanNativeBridge.getInstanceProcAddr()),
        NativePointer.ofAddress(VulkanNativeBridge.getDeviceProcAddr()),
      )

  val surfaceHandle: VulkanHandle
    get() = VulkanHandle.ofBits(VulkanNativeBridge.surface(handle))

  /**
   * Takes the window this context was built for, which is the only one that reaches here: a live
   * context is always presenting through the window being handed back, with only its size changed.
   */
  override fun setSurface(surface: Surface): Boolean = hasSurface

  /**
   * Reports that this context cannot outlive its window. Replacing a `VkSurfaceKHR` needs the
   * outgoing one to still be valid, so the session and this context close together and attach again
   * against the next window, accepting a cold renderer.
   */
  override fun releaseSurface(): Boolean = false

  override fun close() {
    if (handle == 0L) {
      return
    }
    VulkanNativeBridge.destroy(handle)
    handle = 0
  }

  companion object {
    fun create(surface: Surface): VulkanGraphicsContext =
      VulkanGraphicsContext(VulkanNativeBridge.create(surface))
  }
}

private object VulkanNativeBridge {
  init {
    System.loadLibrary("android_map_vulkan")
  }

  external fun create(surface: Surface): Long

  external fun destroy(handle: Long)

  external fun instance(handle: Long): Long

  external fun surface(handle: Long): Long

  external fun physicalDevice(handle: Long): Long

  external fun device(handle: Long): Long

  external fun graphicsQueue(handle: Long): Long

  external fun graphicsQueueFamilyIndex(handle: Long): Int

  external fun getInstanceProcAddr(): Long

  external fun getDeviceProcAddr(): Long
}
