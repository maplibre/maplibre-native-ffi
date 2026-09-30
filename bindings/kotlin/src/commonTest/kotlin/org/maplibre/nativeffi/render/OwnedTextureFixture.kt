package org.maplibre.nativeffi.render

import kotlinx.coroutines.Deferred
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.TimeoutCancellationException
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.currentCoroutineContext
import kotlinx.coroutines.ensureActive
import kotlinx.coroutines.selects.select
import kotlinx.coroutines.withContext
import kotlinx.coroutines.withTimeout
import org.maplibre.nativeffi.MapFixture
import org.maplibre.nativeffi.WAIT_TIMEOUT
import org.maplibre.nativeffi.denyingProvider
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.generated.FrameDemand
import org.maplibre.nativeffi.generated.FrameDemandFlag
import org.maplibre.nativeffi.generated.MapMode
import org.maplibre.nativeffi.generated.RenderDriverKind
import org.maplibre.nativeffi.generated.RenderFrameResult
import org.maplibre.nativeffi.generated.RenderSessionAttachOptions
import org.maplibre.nativeffi.generated.RenderSessionHandle
import org.maplibre.nativeffi.generated.RenderSessionState
import org.maplibre.nativeffi.generated.ResourceProvider
import org.maplibre.nativeffi.generated.Wake
import org.maplibre.nativeffi.runtime.assertCommitted
import org.maplibre.nativeffi.smallMapOptions
import org.maplibre.nativeffi.withMap

/**
 * A caller-driven owned-texture session on a [MapFixture]'s map, serviced on [TestGraphics.thread]
 * and woken by the session's frame and driver-work wakes.
 */
internal class OwnedTextureFixture(
  val mapFixture: MapFixture,
  val session: RenderSessionHandle,
  private val wakes: Channel<Unit>,
) {
  private var nextToken = 1uL

  /** Services the session's driver work on the graphics thread until [deferred] completes. */
  suspend fun <T> complete(deferred: Deferred<T>, what: String = "a driver-serviced operation"): T {
    serviceUntil(what, deferred) { deferred.isCompleted }
    return deferred.await()
  }

  /**
   * Keeps one forced frame demand in flight until [done] holds, and returns the last frame result.
   * A forced demand renders the latest map update whether or not it is new. Each rendered frame is
   * acquired and released so the texture ring never fills, unless [keepLastFrame] leaves the final
   * one for the test to acquire.
   */
  suspend fun renderUntil(
    what: String,
    keepLastFrame: Boolean = false,
    done: (RenderFrameResult?) -> Boolean,
  ): RenderFrameResult? {
    var last: RenderFrameResult? = null
    var pending: ULong? = null
    serviceUntil(what) {
      pending?.let { token ->
        val result = drainResults().lastOrNull { it.token == token }
        if (result != null) {
          last = result
          pending = null
          if (!(keepLastFrame && done(result))) releaseFrames()
        }
      }
      when {
        pending != null -> false
        done(last) -> true
        else -> {
          pending = requestForcedFrame()
          false
        }
      }
    }
    return last
  }

  /** Renders one forced frame and returns its result. */
  /** Renders one forced frame, left for the test to acquire, and returns its result. */
  suspend fun renderFrame(): RenderFrameResult =
    renderUntil("one frame", keepLastFrame = true) { it != null }!!

  /** Loads [json] as the map's style, servicing the session while the command runs. */
  suspend fun setStyle(json: String) {
    assertCommitted(complete(mapFixture.map.setStyleJson(json.encodeToByteArray()), "the style"))
  }

  /** Renders the still image [still] owes a static map, and waits for it to complete. */
  suspend fun renderStill(still: Deferred<Unit> = mapFixture.map.requestStillImage()) {
    renderUntil("a still image") { still.isCompleted }
    still.await()
  }

  private fun releaseFrames() {
    while (true) {
      val frame =
        try {
          session.acquireFrame()
        } catch (error: MaplibreException) {
          if (error.status == MaplibreStatus.NOT_READY) return
          throw error
        }
      frame.release()
    }
  }

  private fun requestForcedFrame(): ULong {
    val token = nextToken++
    session.requestFrame(FrameDemand(flags = FrameDemandFlag(0u), token = token))
    return token
  }

  private fun drainResults(): List<RenderFrameResult> {
    val batch =
      try {
        session.drainFrameResults()
      } catch (error: MaplibreException) {
        if (error.status == MaplibreStatus.NOT_READY) return emptyList()
        throw error
      }
    return batch.use { owner -> List(owner.count().toInt()) { owner.get(it.toULong()) } }
  }

  /**
   * Services driver work on the graphics thread and checks [ready] after each round, blocking on
   * the session's wakes, or on [completion] when it completes off the driver, in between. A wake
   * fires when a queue becomes nonempty, so work that arrives after a round leaves a wake behind
   * for the next receive.
   */
  private suspend fun serviceUntil(
    what: String,
    completion: Deferred<*>? = null,
    ready: () -> Boolean,
  ) {
    withContext(TestGraphics.thread) {
      try {
        withTimeout(WAIT_TIMEOUT) {
          while (true) {
            session.serviceDriverWork(0uL)
            if (ready()) break
            select {
              wakes.onReceive {}
              completion?.onJoin {}
            }
            // A wake that is already queued returns without suspending, so check for the
            // timeout here.
            currentCoroutineContext().ensureActive()
          }
        }
      } catch (timeout: TimeoutCancellationException) {
        throw AssertionError(
          "timed out waiting for $what; session state ${session.getSnapshot().state}",
          timeout,
        )
      }
    }
  }
}

/**
 * Runs [block] against a map with an attached caller-driven owned-texture session of [width] x
 * [height] on the build's backend, and abandons and closes the session afterwards.
 */
internal suspend fun <T> withOwnedTexture(
  width: Int = 32,
  height: Int = 16,
  mapMode: MapMode = MapMode.STATIC,
  textureRingDepth: UInt = 1u,
  provider: ResourceProvider = denyingProvider(),
  block: suspend OwnedTextureFixture.() -> T,
): T =
  withMap(mapMode, provider, smallMapOptions(mapMode, width, height)) {
    val wakes = Channel<Unit>(Channel.CONFLATED)
    val options =
      RenderSessionAttachOptions(
        driver = RenderDriverKind.CALLER_GRAPHICS_THREAD,
        requestedTextureRingDepth = textureRingDepth,
        frameWake = Wake { wakes.trySend(Unit) },
        driverWorkWake = Wake { wakes.trySend(Unit) },
      )
    val attachment =
      withContext(TestGraphics.thread) {
        TestGraphics.attachOwnedTexture(map, width, height, options)
      }
    val fixture = OwnedTextureFixture(this, attachment.session, wakes)
    try {
      fixture.complete(attachment.ready, "the attach")
      fixture.block()
    } finally {
      withContext(NonCancellable + TestGraphics.thread) { attachment.session.abandonAndClose() }
    }
  }

/** Leaves a session closed: an attached session is abandoned rather than leaked. */
internal fun RenderSessionHandle.abandonAndClose() {
  if (isClosed) return
  val state = getSnapshot().state
  if (state != RenderSessionState.DETACHED && state != RenderSessionState.ABANDONED) abandon()
  close()
}
