package org.maplibre.nativeffi.examples.lwjglmap

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.render.FrameDemand
import org.maplibre.nativeffi.render.RenderDriver
import org.maplibre.nativeffi.render.RenderFrameResult
import org.maplibre.nativeffi.render.RenderSessionAttachOptions
import org.maplibre.nativeffi.render.RenderSessionAttachment
import org.maplibre.nativeffi.render.RenderSessionHandle
import org.maplibre.nativeffi.render.RenderTargetExtent

/**
 * The render loop explicitly services caller-driver work on its graphics thread. Native code owns
 * the typed work mailbox and completion state.
 */
internal interface RenderTarget : AutoCloseable {
  fun needsMetalAutoreleasePool(): Boolean = false

  /**
   * Follows a resized host, keeping the session attached so its renderer stays warm. Surface and
   * owned-texture targets resize in place; a caller-owned texture is reallocated at the new size
   * and handed to the live session.
   */
  fun resize(viewport: Viewport)

  /**
   * Renders the latest map update, and reports whether a frame reached the screen. It reports true
   * only for a rendered frame, so the render loop asks for another one when the map has no update
   * yet, the map has not applied the session's size yet, or the target had no frame to draw into.
   */
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

    /**
     * Detaches a session whose handover failed, before the targets it may hold are released. A
     * failed handover leaves it unknown which target the session holds.
     */
    fun detachSuppressed(error: RuntimeException, session: RenderSessionHandle) {
      try {
        completeDriverOperation(session, session.detach())
      } catch (cleanupError: Exception) {
        error.addSuppressed(cleanupError)
      }
    }

    val callerDriverOptions: RenderSessionAttachOptions =
      RenderSessionAttachOptions(driver = RenderDriver.CALLER_GRAPHICS_THREAD)

    fun completeAttachment(attachment: RenderSessionAttachment): RenderSessionHandle {
      try {
        completeDriverOperation(attachment.session, attachment.completed)
        return attachment.session
      } catch (error: Throwable) {
        runCatching { attachment.session.abandon() }
        runCatching { attachment.session.close() }
        throw error
      }
    }

    fun completeDriverOperation(session: RenderSessionHandle, completed: Deferred<Unit>) {
      while (!completed.isCompleted) session.serviceDriverWork()
      runSuspend { completed.await() }
    }

    fun renderFrame(session: RenderSessionHandle): RenderFrameResult? {
      session.requestFrame(FrameDemand(present = true))
      session.serviceDriverWork()
      return session.drainFrameResults().lastOrNull()
    }

    fun closeSession(session: RenderSessionHandle) {
      completeDriverOperation(session, session.detach())
      session.close()
    }

    fun closeSuppressed(error: RuntimeException, closeable: AutoCloseable?) {
      if (closeable == null) {
        return
      }
      try {
        if (closeable is RenderSessionHandle) closeSession(closeable) else closeable.close()
      } catch (cleanupError: Exception) {
        error.addSuppressed(cleanupError)
      }
    }
  }
}
