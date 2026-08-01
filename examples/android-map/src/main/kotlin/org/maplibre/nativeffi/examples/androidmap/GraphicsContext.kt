package org.maplibre.nativeffi.examples.androidmap

import android.view.Surface

/**
 * The host graphics context.
 *
 * A `SurfaceView` destroys and recreates its surface across a rotation and across a trip to the
 * background, while the map goes on living. A context whose handles outlive that keeps presenting
 * through whatever surface it holds now, so the render session attached against it stays live and
 * keeps its renderer. A context built from the surface itself says so instead, and the caller
 * closes the session with it.
 */
internal interface GraphicsContext : AutoCloseable {
  val backendName: String

  /** Whether this context presents through a live host surface, so a frame has somewhere to go. */
  val hasSurface: Boolean

  /**
   * Presents through the host surface the platform just handed over, reporting whether this context
   * took it.
   *
   * `false` means this context cannot present through it, either because its handles belong to the
   * surface it was built for or because the graphics API context itself is gone. The caller then
   * closes the session attached against this context, closes this context, and builds both again,
   * accepting a cold renderer.
   */
  fun setSurface(surface: Surface): Boolean

  /**
   * Releases the host surface the platform is taking back, reporting whether this context outlived
   * it.
   *
   * `true` leaves this context presenting somewhere harmless until a surface returns, so a session
   * attached against it stays live and keeps its renderer across the gap. `false` means the surface
   * and this context share one lifetime, so the caller closes the session and this context
   * together.
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
