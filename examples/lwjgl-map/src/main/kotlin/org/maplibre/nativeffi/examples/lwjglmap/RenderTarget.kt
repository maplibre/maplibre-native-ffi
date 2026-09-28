package org.maplibre.nativeffi.examples.lwjglmap

import kotlinx.coroutines.Deferred
import kotlinx.coroutines.runBlocking
import org.maplibre.nativeffi.generated.FrameDemandFlag
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.LogicalExtent
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.RenderDriverKind
import org.maplibre.nativeffi.generated.RenderFrameResult
import org.maplibre.nativeffi.generated.RenderSessionAttachOptions
import org.maplibre.nativeffi.generated.RenderSessionHandle
import org.maplibre.nativeffi.generated.RenderTargetExtent

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
      RenderTargetExtent(
        viewport.width().toUInt(),
        viewport.height().toUInt(),
        viewport.scaleFactor(),
      )

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
      RenderSessionAttachOptions(driver = RenderDriverKind.CALLER_GRAPHICS_THREAD)

    /** A session-owned texture ring deep enough to keep compositing while the map renders. */
    val ownedTextureOptions: RenderSessionAttachOptions =
      RenderSessionAttachOptions(
        driver = RenderDriverKind.CALLER_GRAPHICS_THREAD,
        requestedTextureRingDepth = 2u,
      )

    fun finishAttachment(session: RenderSessionHandle, ready: Deferred<Unit>): RenderSessionHandle {
      try {
        completeDriverOperation(session, ready)
        return session
      } catch (error: Throwable) {
        runCatching { session.abandon() }
        runCatching { session.close() }
        throw error
      }
    }

    fun completeDriverOperation(session: RenderSessionHandle, completed: Deferred<*>) {
      while (!completed.isCompleted) session.serviceDriverWork(0uL)
      val result = runBlocking { completed.await() }
      if (result is org.maplibre.nativeffi.runtime.CommandCompletion) {
        check(result.status == org.maplibre.nativeffi.error.MaplibreStatus.OK) {
          "Driver operation failed: ${result.status}: ${result.diagnostic}"
        }
      }
    }

    /**
     * Resizes a map whose session cannot carry the extent itself. A caller-owned texture is sized
     * by this host, so its handover replaces only the graphics resource.
     */
    fun resizeMap(map: MapHandle, viewport: Viewport) {
      map.resize(
        LogicalExtent(viewport.width().toUInt(), viewport.height().toUInt(), viewport.scaleFactor())
      )
    }

    /** Submits one host-paced demand and reports the frame the driver produced for it. */
    fun renderFrame(session: RenderSessionHandle): RenderFrameResult? {
      session.requestFrame(
        GeneratedApi.frameDemandDefault()
          .copy(
            flags =
              FrameDemandFlag(
                FrameDemandFlag.IF_NEEDED.rawValue or FrameDemandFlag.PRESENT.rawValue
              )
          )
      )
      session.serviceDriverWork(0uL)
      val batch =
        try {
          session.drainFrameResults()
        } catch (error: org.maplibre.nativeffi.error.MaplibreException) {
          if (error.status == org.maplibre.nativeffi.error.MaplibreStatus.NOT_READY) return null
          throw error
        }
      return batch.use { owner ->
        val count = owner.count()
        if (count == 0uL) null else owner.get(count - 1uL)
      }
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
