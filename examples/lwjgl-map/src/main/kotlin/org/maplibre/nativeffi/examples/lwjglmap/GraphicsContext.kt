package org.maplibre.nativeffi.examples.lwjglmap

import org.maplibre.nativeffi.generated.RenderBackendFlag

internal interface GraphicsContext : AutoCloseable {
  fun window(): Long

  fun backend(): RenderBackendFlag

  fun resize(viewport: Viewport) {}

  override fun close()

  companion object {
    @JvmStatic
    fun create(
      title: String,
      width: Int,
      height: Int,
      backends: RenderBackendFlag,
      visible: Boolean = true,
    ): GraphicsContext {
      if (backends.contains(RenderBackendFlag.METAL)) {
        return MetalContext.create(title, width, height, visible)
      }
      if (backends.contains(RenderBackendFlag.OPENGL)) {
        return OpenGLContext.create(title, width, height, visible)
      }
      if (backends.contains(RenderBackendFlag.VULKAN)) {
        return VulkanContext.create(title, width, height, visible)
      }
      throw IllegalStateException(
        "The loaded MapLibre native library does not support a backend usable by lwjgl-map"
      )
    }
  }
}
