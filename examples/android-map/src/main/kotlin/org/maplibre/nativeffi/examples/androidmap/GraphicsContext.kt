package org.maplibre.nativeffi.examples.androidmap

import android.view.Surface

/**
 * The host graphics context. A `SurfaceView` destroys and recreates its surface across a rotation
 * and a trip to the background, and a context that outlives that keeps the render session attached
 * against it live.
 */
internal interface GraphicsContext : AutoCloseable {
  val backendName: String

  /** Whether this context presents through a live host surface. */
  val hasSurface: Boolean

  /**
   * Presents through the host surface the platform just handed over, reporting whether this context
   * took it. False means the caller closes the session and this context and builds both again,
   * accepting a cold renderer.
   */
  fun setSurface(surface: Surface): Boolean

  /**
   * Releases the host surface the platform is taking back, reporting whether this context outlived
   * it. True leaves a session attached against it live until a surface returns; false means the
   * caller closes the session and this context together.
   */
  fun releaseSurface(): Boolean

  companion object {
    fun create(surface: Surface): GraphicsContext =
      when (BuildConfig.RENDER_BACKEND) {
        "opengl" -> EglGraphicsContext.create(surface)
        "vulkan" -> VulkanGraphicsContext.create(surface)
        else -> error("Unsupported render backend: ${BuildConfig.RENDER_BACKEND}")
      }
  }
}
