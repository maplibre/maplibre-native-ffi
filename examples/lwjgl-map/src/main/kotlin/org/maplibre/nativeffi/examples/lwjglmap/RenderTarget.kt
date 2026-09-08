package org.maplibre.nativeffi.examples.lwjglmap

import kotlinx.coroutines.Deferred
import kotlinx.coroutines.runBlocking
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapSize
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
   * Renders the latest map update, and reports whether the render loop may rest. It reports false
   * when no frame reached the screen and when the map asked for another frame while this one
   * rendered, so the loop demands one more after its idle wait.
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
     * Releases a session whose handover failed, before the targets it may hold are released. A
     * failed handover leaves it unknown which target the session holds. A detach that fails falls
     * back to abandonment, so the caller may close the session afterwards either way.
     */
    fun detachSuppressed(error: RuntimeException, session: RenderSessionHandle) {
      try {
        completeDriverOperation(session, session.detach())
      } catch (cleanupError: Exception) {
        error.addSuppressed(cleanupError)
        runCatching { session.abandon() }.onFailure(error::addSuppressed)
      }
    }

    val callerDriverOptions: RenderSessionAttachOptions =
      RenderSessionAttachOptions(driver = RenderDriver.CALLER_GRAPHICS_THREAD)

    /** A session-owned texture ring deep enough to keep compositing while the map renders. */
    val ownedTextureOptions: RenderSessionAttachOptions =
      RenderSessionAttachOptions(
        driver = RenderDriver.CALLER_GRAPHICS_THREAD,
        requestedTextureRingDepth = 2,
      )

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
      runBlocking { completed.await() }
    }

    /**
     * Resizes a map whose session cannot carry the extent itself. A caller-owned texture is sized
     * by this host, so its handover replaces only the graphics resource.
     */
    fun resizeMap(map: MapHandle, viewport: Viewport) {
      map.resize(MapSize(viewport.width(), viewport.height(), viewport.scaleFactor()))
    }

    /** Submits one host-paced demand and reports the frame the driver produced for it. */
    fun renderFrame(session: RenderSessionHandle): RenderFrameResult? {
      session.requestFrame(FrameDemand(present = true))
      session.serviceDriverWork()
      return session.drainFrameResults().lastOrNull()
    }

    /**
     * Closes a session, detaching it first unless a failed handover already released it. Detaching
     * a released session reports an invalid state, and a close that runs from a `finally` would
     * replace the handover failure with that.
     */
    fun closeSession(session: RenderSessionHandle, released: Boolean = false) {
      if (!released) completeDriverOperation(session, session.detach())
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
