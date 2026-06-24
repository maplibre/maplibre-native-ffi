package org.maplibre.nativeffi.examples.composemap.surface

internal object MacMetalBridgeNative {
  private const val MTL_TEXTURE_TYPE_2D = 2L
  private const val MTL_PIXEL_FORMAT_BGRA8_UNORM = 80L
  private const val MTL_TEXTURE_USAGE_SHADER_READ = 1L
  private const val MTL_TEXTURE_USAGE_RENDER_TARGET = 4L
  private const val MTL_STORAGE_MODE_PRIVATE = 2L

  fun metalAdapter(metalDevice: Long): Long =
    MacObjectiveC.autoreleasePool().use { MacObjectiveC.sendPointer(metalDevice, "adapter") }

  fun createMetalTexture(metalDevice: Long, oldTexture: Long, width: Int, height: Int): Long =
    MacObjectiveC.autoreleasePool().use {
      if (oldTexture != 0L) {
        val oldWidth = MacObjectiveC.sendLong(oldTexture, "width")
        val oldHeight = MacObjectiveC.sendLong(oldTexture, "height")
        if (oldWidth == width.toLong() && oldHeight == height.toLong()) {
          return oldTexture
        }
      }

      val adapter = MacObjectiveC.sendPointer(metalDevice, "adapter")
      val descriptor = MacObjectiveC.allocInit("MTLTextureDescriptor")
      try {
        MacObjectiveC.sendVoid(descriptor, "setTextureType:", MTL_TEXTURE_TYPE_2D)
        MacObjectiveC.sendVoid(descriptor, "setPixelFormat:", MTL_PIXEL_FORMAT_BGRA8_UNORM)
        MacObjectiveC.sendVoid(descriptor, "setWidth:", width.toLong())
        MacObjectiveC.sendVoid(descriptor, "setHeight:", height.toLong())
        MacObjectiveC.sendVoid(
          descriptor,
          "setUsage:",
          MTL_TEXTURE_USAGE_SHADER_READ or MTL_TEXTURE_USAGE_RENDER_TARGET,
        )
        MacObjectiveC.sendVoid(descriptor, "setStorageMode:", MTL_STORAGE_MODE_PRIVATE)
        val texture = MacObjectiveC.sendPointer(adapter, "newTextureWithDescriptor:", descriptor)
        if (texture == 0L) {
          throw NativeSurfaceBridgeException("Skiko Metal texture allocation returned null")
        }
        texture
      } finally {
        MacObjectiveC.release(descriptor)
      }
    }

  fun disposeMetalTexture(texture: Long) {
    MacObjectiveC.autoreleasePool().use { MacObjectiveC.release(texture) }
  }

  fun texturePixelFormat(texture: Long): Long =
    MacObjectiveC.autoreleasePool().use {
      if (texture == 0L) 0L else MacObjectiveC.sendLong(texture, "pixelFormat")
    }

  fun <T> runInAutoreleasePool(action: () -> T): T =
    MacObjectiveC.autoreleasePool().use { action() }
}
