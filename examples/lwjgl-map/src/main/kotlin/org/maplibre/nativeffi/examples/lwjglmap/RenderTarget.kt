package org.maplibre.nativeffi.examples.lwjglmap

import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.render.RenderTargetExtent

/**
 * The render session and its mode-specific resources.
 *
 * Attaching records the calling thread as the session's owner, so every render target is created,
 * driven, and closed on the render loop thread, where the host graphics context lives.
 */
internal interface RenderTarget : AutoCloseable {
  fun needsMetalAutoreleasePool(): Boolean = false

  /**
   * Follows a resized host, keeping the session attached.
   *
   * Surface and owned-texture targets are sized by the session, so they resize in place. A
   * caller-owned texture is sized by this example instead, so it allocates one at the new size and
   * hands it to the live session. Either way the session keeps its renderer, which is what keeps
   * the map from going cold on every window resize.
   */
  fun resize(viewport: Viewport)

  /** Renders the latest map update. Returns false when no update is available yet. */
  fun renderUpdate(): Boolean

  override fun close()

  companion object {
    /** Attaches a render session for the active graphics API and mode, on the calling thread. */
    fun attach(
      graphics: GraphicsContext,
      map: MapHandle,
      viewport: Viewport,
      mode: RenderTargetMode,
    ): RenderTarget =
      when (graphics) {
        is MetalContext -> MetalRenderTarget.attach(graphics, map, viewport, mode)
        is VulkanContext -> VulkanRenderTarget.attach(graphics, map, viewport, mode)
        is OpenGLContext -> OpenGLRenderTarget.attach(graphics, map, viewport, mode)
        else -> error("Unsupported graphics context: ${graphics.backend()}")
      }

    fun extent(viewport: Viewport): RenderTargetExtent =
      RenderTargetExtent(viewport.width(), viewport.height(), viewport.scaleFactor())

    fun closeSuppressed(error: RuntimeException, closeable: AutoCloseable?) {
      if (closeable == null) {
        return
      }
      try {
        closeable.close()
      } catch (cleanupError: Exception) {
        error.addSuppressed(cleanupError)
      }
    }
  }
}
