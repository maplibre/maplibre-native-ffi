package org.maplibre.nativeffi.render

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions
import org.maplibre.nativeffi.sleepMillis

/** Backend texture size read from an acquired frame. */
internal data class OwnedTextureFrameSize(val width: Int, val height: Int)

/** Backend-owned texture session plus the graphics context that keeps it valid. */
internal interface OwnedTextureTestSession : AutoCloseable {
  /** The attached session and the deferred result of its attachment. */
  val attachment: RenderSessionAttachment

  val session: RenderSessionHandle
    get() = attachment.session

  /**
   * Attaches a second owned-texture session on [session]'s map, reusing this fixture's graphics
   * context. Native rejects the second attach; the fixture keeps the live context current.
   */
  fun attachAnotherOwnedTexture(width: Int, height: Int): RenderSessionAttachment

  /** Reads the backend texture size behind an acquired frame. */
  fun frameSize(frame: AcquiredFrameHandle): OwnedTextureFrameSize
}

/**
 * Attaches a caller-graphics-thread owned-texture session for common render tests.
 *
 * Returns null when this source set has no fixture for the loaded native backend, so the common
 * session tests stay compiled on every target.
 */
internal expect object OwnedTextureTestSupport {
  fun attach(map: MapHandle, width: Int, height: Int): OwnedTextureTestSession?
}

/** Attach options every fixture uses: the fixture's context is current on the calling thread. */
internal val OWNED_TEXTURE_ATTACH_OPTIONS: RenderSessionAttachOptions =
  RenderSessionAttachOptions(driver = RenderDriver.CALLER_GRAPHICS_THREAD)

/** Completes [deferred] while servicing the driver work only this thread can run. */
internal suspend fun <T> RenderSessionHandle.completeOnDriver(deferred: Deferred<T>): T {
  while (!deferred.isCompleted) serviceDriverWork()
  return deferred.await()
}

/** Demands one frame and services driver work until the demand reaches a terminal result. */
internal fun RenderSessionHandle.renderOneFrame(
  demand: FrameDemand = FrameDemand(ifNeeded = false)
): RenderFrameResult {
  requestFrame(demand)
  while (true) {
    serviceDriverWork()
    val results = drainFrameResults()
    if (results.isNotEmpty()) return results.last()
  }
}

/** Renders until the map settles, and returns the frame that asked for no repaint. */
internal fun RenderSessionHandle.renderUntilSettled(attempts: Int = 500): RenderFrameResult {
  var last: RenderFrameResult? = null
  repeat(attempts) {
    val result = renderOneFrame()
    if (result.disposition == RenderResult.RENDERED && !result.needsRepaint) return result
    last = result
    // The map applies a new extent and its style on the runtime worker, so leave that
    // worker room to publish between demands.
    sleepMillis(1)
  }
  error("the map never settled; last frame result: $last")
}

/**
 * Demands one frame from a core-worker session and waits for the map to render it.
 *
 * A core-worker session renders on the runtime, so the caller only polls the frame-result queue.
 * SIZE_PENDING means the map has not applied this target's extent yet.
 */
internal fun RenderSessionHandle.awaitRenderedFrame(attempts: Int = 1000): RenderFrameResult {
  var last: RenderFrameResult? = null
  repeat(attempts) {
    requestFrame(FrameDemand(ifNeeded = false))
    val results = drainFrameResults()
    if (results.isNotEmpty()) {
      val result = results.last()
      if (result.disposition == RenderResult.RENDERED) return result
      last = result
    }
    sleepMillis(1)
  }
  error("the session never rendered a frame; last frame result: $last")
}

/** Leaves a session closable: an attached session is abandoned rather than leaked. */
internal fun RenderSessionHandle.abandonAndClose() {
  if (isClosed) return
  val state = snapshot().state
  if (state != RenderSessionState.DETACHED && state != RenderSessionState.ABANDONED) {
    abandon()
  }
  close()
}

/**
 * Runs [block] against a fresh runtime, map, and owned-texture session.
 *
 * Teardown always runs, and a teardown failure never hides a failure from [block].
 */
internal suspend fun withOwnedTextureSession(
  width: Int = 32,
  height: Int = 16,
  mapWidth: Int = width,
  mapHeight: Int = height,
  mapMode: MapMode = MapMode.CONTINUOUS,
  block: suspend (RuntimeHandle, MapHandle, OwnedTextureTestSession) -> Unit,
) {
  val failures = mutableListOf<Throwable>()
  val runtime = RuntimeHandle.create(RuntimeOptions())
  val map =
    MapHandle.create(
        runtime,
        MapOptions().apply {
          this.width = mapWidth
          this.height = mapHeight
          this.mapMode = mapMode
        },
      )
      .await()
  val owned = OwnedTextureTestSupport.attach(map, width, height)
  if (owned != null) {
    try {
      owned.session.completeOnDriver(owned.attachment.completed)
      block(runtime, map, owned)
    } catch (error: Throwable) {
      failures += error
    }
    failures.addIfFailed { owned.close() }
    // Session teardown finishes on the runtime worker, so the map owns no session by the
    // time the map itself closes.
    failures.addIfFailed { runtime.barrier().await() }
  }
  failures.addIfFailed { map.close() }
  failures.addIfFailed { runtime.close() }
  failures.firstOrNull()?.let { throw it }
}

private inline fun MutableList<Throwable>.addIfFailed(block: () -> Unit) {
  runCatching(block).exceptionOrNull()?.let { add(it) }
}
