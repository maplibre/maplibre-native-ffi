package org.maplibre.nativeffi.examples.androidmap

import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor
import org.maplibre.nativeffi.render.RenderSessionHandle
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor

/**
 * The native-surface render target.
 *
 * The thread that attaches a session owns it for the session's lifetime, so every method here runs
 * on the render loop thread (the UI thread), where the host surface and its graphics context live.
 */
internal class SurfaceRenderTarget private constructor(private val session: RenderSessionHandle) :
  AutoCloseable {
  fun renderUpdate(): Boolean = session.renderUpdate()

  override fun close() {
    session.close()
  }

  companion object {
    fun attach(map: MapHandle, graphics: GraphicsContext, viewport: Viewport): SurfaceRenderTarget =
      when (graphics) {
        is EglGraphicsContext -> {
          val descriptor =
            OpenGLSurfaceDescriptor(viewport.extent, graphics.descriptor, graphics.surfacePointer)
          SurfaceRenderTarget(map.attachOpenGLSurface(descriptor))
        }
        is VulkanGraphicsContext -> {
          val descriptor =
            VulkanSurfaceDescriptor(viewport.extent, graphics.descriptor, graphics.surfacePointer)
          SurfaceRenderTarget(map.attachVulkanSurface(descriptor))
        }
        else -> error("Unsupported graphics context: ${graphics::class.java.name}")
      }
  }
}
