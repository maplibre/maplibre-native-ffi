package org.maplibre.nativeffi.examples.androidmap

import android.view.Surface
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.VulkanContextDescriptor

internal class VulkanGraphicsContext private constructor(private var handle: Long) :
  GraphicsContext {
  override val backendName: String = "vulkan"

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

  val surfacePointer: NativePointer
    get() = NativePointer.ofAddress(VulkanNativeBridge.surface(handle))

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
