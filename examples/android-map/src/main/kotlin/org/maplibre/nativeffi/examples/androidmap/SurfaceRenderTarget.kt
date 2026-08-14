package org.maplibre.nativeffi.examples.androidmap

import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor
import org.maplibre.nativeffi.render.RenderResult
import org.maplibre.nativeffi.render.RenderSessionHandle
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor

/**
 * The native-surface render target. The thread that attaches a session owns it for the session's
 * lifetime, so every method here runs on the UI thread, where the host surface lives.
 */
internal class SurfaceRenderTarget private constructor(private val session: RenderSessionHandle) :
  AutoCloseable {
  fun renderUpdate(): Boolean = session.renderUpdate().result == RenderResult.RENDERED

  /**
   * Follows the host viewport and the surface [graphics] presents through now, keeping the session
   * and its renderer.
   *
   * The OpenGL path carries a resize and a surface replacement through one call, because the
   * surface is made current on the next render and may name a replacement for one the platform
   * already destroyed. A Vulkan session destroys its swapchain before taking a new surface, which
   * needs the outgoing surface to still be valid, so only a resize reaches it here.
   */
  fun resize(graphics: GraphicsContext, viewport: Viewport) {
    when (graphics) {
      is EglGraphicsContext ->
        session.setOpenGLSurfaceTarget(
          OpenGLSurfaceDescriptor(viewport.extent, graphics.descriptor, graphics.surfacePointer)
        )
      is VulkanGraphicsContext ->
        session.resize(viewport.logicalWidth, viewport.logicalHeight, viewport.scaleFactor)
      else -> error("Unsupported graphics context: ${graphics::class.java.name}")
    }
  }

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
