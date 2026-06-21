package org.maplibre.nativeffi.examples.lwjglmap

import java.util.Locale
import org.maplibre.nativeffi.render.RenderBackend

internal interface GraphicsContext : AutoCloseable {
  fun window(): Long

  fun backend(): RenderBackend

  fun resize(viewport: Viewport) {}

  override fun close()

  companion object {
    @JvmStatic
    fun create(
      title: String,
      width: Int,
      height: Int,
      backends: Set<RenderBackend>,
    ): GraphicsContext {
      if (isMac() && backends.contains(RenderBackend.METAL)) {
        return MetalContext.create(title, width, height)
      }
      if (!isMac() && backends.contains(RenderBackend.OPENGL)) {
        return OpenGLContext.create(title, width, height)
      }
      if (backends.contains(RenderBackend.VULKAN)) {
        return VulkanContext.create(title, width, height)
      }
      throw IllegalStateException(
        "The loaded MapLibre native library does not support a backend usable by lwjgl-map on this platform"
      )
    }

    @JvmStatic
    fun isMac(): Boolean = System.getProperty("os.name").lowercase(Locale.ROOT).contains("mac")
  }
}
