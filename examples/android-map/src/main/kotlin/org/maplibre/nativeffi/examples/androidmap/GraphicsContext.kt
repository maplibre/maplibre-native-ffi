package org.maplibre.nativeffi.examples.androidmap

import android.view.Surface

internal interface GraphicsContext : AutoCloseable {
  val backendName: String

  companion object {
    fun create(surface: Surface): GraphicsContext =
      when (BuildConfig.RENDER_BACKEND) {
        "opengl" -> EglGraphicsContext.create(surface)
        "vulkan" -> VulkanGraphicsContext.create(surface)
        else -> error("Unsupported render backend: ${BuildConfig.RENDER_BACKEND}")
      }
  }
}
