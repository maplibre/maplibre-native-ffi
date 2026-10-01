package org.maplibre.nativeffi.examples.lwjglmap

import kotlinx.coroutines.Deferred
import kotlinx.coroutines.runBlocking
import org.lwjgl.glfw.GLFW.glfwPostEmptyEvent
import org.lwjgl.glfw.GLFW.glfwWaitEvents
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.generated.FrameDemandFlag
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.LogicalExtent
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.RenderResult
import org.maplibre.nativeffi.generated.RenderSessionAttachment
import org.maplibre.nativeffi.generated.RenderSessionHandle
import org.maplibre.nativeffi.generated.RenderTargetExtent

/**
 * A render session that the GLFW thread drives. The render loop requests frames, services driver
 * work after its wake, and drains frame results after theirs. This class serves a native surface,
 * where the driver presents each frame; texture targets compose each rendered frame into the window
 * in [present].
 */
internal open class RenderTarget(protected val session: RenderSessionHandle) : AutoCloseable {
  open fun needsMetalAutoreleasePool(): Boolean = false

  /** Asks for a frame. A forced frame renders even when the map has no newer update. */
  fun requestFrame(force: Boolean = false) {
    val flags =
      if (force) FrameDemandFlag.PRESENT else FrameDemandFlag.IF_NEEDED or FrameDemandFlag.PRESENT
    session.requestFrame(GeneratedApi.frameDemandDefault().copy(flags = flags))
  }

  fun serviceDriverWork() {
    session.serviceDriverWork(0uL)
  }

  /**
   * Drains every frame result and presents each rendered frame, reporting whether one reached the
   * window. A result that asks for another frame, as during a paint transition, requests it. A
   * target that was not ready, or a frame that missed the window, consumed its map update, so a
   * paced retry forces the next frame.
   */
  fun drainFrameResults(): Boolean {
    val batch =
      try {
        session.drainFrameResults()
      } catch (error: MaplibreException) {
        if (error.status == MaplibreStatus.NOT_READY) return false
        throw error
      }
    var presented = false
    var missed = false
    var needsRepaint = false
    batch.use { results ->
      for (index in 0uL until results.count()) {
        val result = results.get(index)
        when (result.disposition) {
          RenderResult.RENDERED -> if (present()) presented = true else missed = true
          RenderResult.TARGET_NOT_READY -> missed = true
          else -> continue
        }
        needsRepaint = needsRepaint || result.needsRepaint
      }
    }
    if (missed) {
      retryAtNanos = System.nanoTime() + RETRY_DELAY_NANOS
    } else if (needsRepaint) {
      requestFrame()
    }
    return presented
  }

  /** When a paced retry is due, as a [System.nanoTime] value, or null with none pending. */
  var retryAtNanos: Long? = null
    private set

  /** Forces the pending paced retry once it is due. */
  fun retryIfDue() {
    val due = retryAtNanos ?: return
    if (System.nanoTime() < due) return
    retryAtNanos = null
    requestFrame(force = true)
  }

  /**
   * Shows the frame the session just rendered, and reports whether it reached the window. A native
   * surface has already presented it.
   */
  protected open fun present(): Boolean = true

  /**
   * Follows a resized host and keeps the session attached, so its renderer stays warm. The session
   * resize carries the new extent to the map.
   */
  open fun resize(viewport: Viewport) {
    session.resize(extent(viewport))
  }

  /**
   * Hands the session a replacement texture through [handover], and waits for it. Frames that ran
   * before the handover drew into the outgoing texture, so they are drained and presented while
   * that texture is still current. A failed handover leaves it unknown which texture the session
   * holds, so the session is detached before the caller releases either one.
   */
  protected fun handOver(handover: () -> Deferred<Unit>) {
    try {
      awaitDriverWork(session, handover())
    } catch (error: RuntimeException) {
      released = true
      try {
        awaitDriverWork(session, session.detach())
      } catch (cleanupError: RuntimeException) {
        error.addSuppressed(cleanupError)
        runCatching { session.abandon() }.onFailure(error::addSuppressed)
      }
      throw error
    }
    drainFrameResults()
  }

  /** Set once a failed handover released the session, so [close] only retires the handle. */
  private var released = false

  /** Detaches through the driver, abandoning the session if that fails, then closes it. */
  override fun close() {
    try {
      if (!released) awaitDriverWork(session, session.detach())
    } catch (error: RuntimeException) {
      runCatching { session.abandon() }.onFailure(error::addSuppressed)
      session.close()
      throw error
    }
    session.close()
  }

  companion object {
    /** A session-owned texture ring deep enough to keep compositing while the map renders. */
    const val OWNED_TEXTURE_RING_DEPTH = 2u

    /** How long a frame that did not reach the window waits to retry, about one refresh. */
    const val RETRY_DELAY_NANOS = 16_000_000L

    /** Attaches a render session for the active graphics API and mode, on the GLFW thread. */
    fun attach(
      graphics: GraphicsContext,
      map: MapHandle,
      viewport: Viewport,
      mode: RenderTargetMode,
      wakes: LoopWakes,
    ): RenderTarget =
      when (graphics) {
        is MetalContext -> MetalRenderTarget.attach(graphics, map, viewport, mode, wakes)
        is VulkanContext -> VulkanRenderTarget.attach(graphics, map, viewport, mode, wakes)
        is OpenGLContext -> OpenGLRenderTarget.attach(graphics, map, viewport, mode, wakes)
        else -> error("Unsupported graphics context: ${graphics.backend()}")
      }

    fun extent(viewport: Viewport): RenderTargetExtent =
      RenderTargetExtent(
        viewport.width().toUInt(),
        viewport.height().toUInt(),
        viewport.scaleFactor(),
      )

    /**
     * Resizes a map whose session carries no extent: a texture handover replaces only the texture.
     */
    fun resizeMap(map: MapHandle, viewport: Viewport) {
      map.resize(
        LogicalExtent(viewport.width().toUInt(), viewport.height().toUInt(), viewport.scaleFactor())
      )
    }

    /** Services the attachment until it completes, and returns the attached session. */
    fun attached(attachment: RenderSessionAttachment): RenderSessionHandle {
      try {
        awaitDriverWork(attachment.session, attachment.ready)
        return attachment.session
      } catch (error: Throwable) {
        runCatching { attachment.session.abandon() }
        runCatching { attachment.session.close() }
        throw error
      }
    }

    /**
     * Services driver work on the GLFW thread until [completion] finishes. Between services the
     * thread waits for the driver-work wake or for the completion, each of which posts an empty
     * event. This must not run inside a GLFW callback.
     */
    fun <T> awaitDriverWork(session: RenderSessionHandle, completion: Deferred<T>): T {
      completion.invokeOnCompletion { glfwPostEmptyEvent() }
      while (true) {
        session.serviceDriverWork(0uL)
        if (completion.isCompleted) break
        glfwWaitEvents()
      }
      return runBlocking { completion.await() }
    }

    fun closeSuppressed(error: Throwable, closeable: AutoCloseable?) {
      try {
        closeable?.close()
      } catch (cleanupError: Exception) {
        error.addSuppressed(cleanupError)
      }
    }
  }
}
