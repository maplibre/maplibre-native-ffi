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

  /**
   * Follows the host viewport and the surface [graphics] presents through now.
   *
   * The platform destroys and recreates a `SurfaceView`'s surface while the map goes on living, so
   * following it means handing the live session whatever surface its graphics context holds now
   * rather than closing the session. The session keeps its renderer across that, and with it the
   * tile pyramid, glyph and image atlases, symbol placement, and feature state, so a rotation or a
   * return from the background comes back to a warm map. A scale factor change is the exception the
   * C API documents, starting a new renderer for the new pixel ratio.
   *
   * The OpenGL path carries a resize and a surface replacement through one call: the extent applies
   * as a resize applies one, and the surface is made current on the next render, which is what lets
   * it name a replacement for a surface the platform has already destroyed. The surface a Vulkan
   * session presents through cannot be replaced this way, because the session destroys its
   * swapchain before taking the new one and the outgoing surface has to still be valid for that;
   * only a resized window reaches here, and the session resizes against the surface it has.
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
