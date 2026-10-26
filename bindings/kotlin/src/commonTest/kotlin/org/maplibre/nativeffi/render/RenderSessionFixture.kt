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
import org.maplibre.nativeffi.FrameDemand
import org.maplibre.nativeffi.FrameDemandFlag
import org.maplibre.nativeffi.MapFixture
import org.maplibre.nativeffi.MapMode
import org.maplibre.nativeffi.NewRenderSession
import org.maplibre.nativeffi.RenderDriverKind
import org.maplibre.nativeffi.RenderFrameResult
import org.maplibre.nativeffi.RenderSessionAttachOptions
import org.maplibre.nativeffi.RenderSessionCapabilityFlag
import org.maplibre.nativeffi.RenderSessionHandle
import org.maplibre.nativeffi.RenderSessionState
import org.maplibre.nativeffi.ResourceProvider
import org.maplibre.nativeffi.RuntimeOptions
import org.maplibre.nativeffi.WAIT_TIMEOUT
import org.maplibre.nativeffi.Wake
import org.maplibre.nativeffi.denyingProvider
import org.maplibre.nativeffi.smallMapOptions
import org.maplibre.nativeffi.withMap

/**
 * A render session on a [MapFixture]'s map, woken by the session's frame and driver-work wakes. A
 * caller-driven session is serviced on [TestGraphics.thread]; a core worker drives its own.
 */
internal class RenderSessionFixture(
  val mapFixture: MapFixture,
  val session: RenderSessionHandle,
  private val wakes: Channel<Unit>,
  driver: RenderDriverKind,
) {
  private var nextToken = 1uL
  private val callerDriven = driver == RenderDriverKind.CALLER_GRAPHICS_THREAD
  // A private texture and a surface hand out no frames, so their rendered frames need no release.
  private val acquiresFrames =
    RenderSessionCapabilityFlag.FRAME_ACQUISITION in session.getCapabilities().flags

  /** Services the session's driver work on the graphics thread until [deferred] completes. */
  suspend fun <T> complete(deferred: Deferred<T>, what: String = "a driver-serviced operation"): T {
    serviceUntil(what, deferred) { deferred.isCompleted }
    return deferred.await()
  }

  /**
   * Keeps one forced frame demand in flight until [done] holds, and returns the last frame result.
   * A forced demand renders the latest map update whether or not it is new. Where the session hands
   * out frames, each rendered frame is acquired and released so the texture ring never fills,
   * unless [keepLastFrame] leaves the final one for the test to acquire.
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
          if (acquiresFrames && !(keepLastFrame && done(result))) releaseFrames()
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

  /** Renders one forced frame, left for the test to acquire, and returns its result. */
  suspend fun renderFrame(): RenderFrameResult =
    renderUntil("one frame", keepLastFrame = true) { it != null }!!

  /** Loads [json] as the map's style, servicing the session while the command runs. */
  suspend fun setStyle(json: String) {
    complete(mapFixture.map.setStyleJson(json), "the style")
  }

  /**
   * Services the session until the still image [still] owes a static map completes and the result
   * of the demand that rendered it arrives. The request submits that demand itself, and it is the
   * only one in flight, since [renderUntil] returns with none of its own. Where the session hands
   * out frames, each rendered frame is acquired and released so the texture ring never fills.
   */
  suspend fun renderStill(still: Deferred<Unit> = mapFixture.map.requestStillImage()) {
    var rendered = false
    serviceUntil("a still image", still) {
      if (drainResults().isNotEmpty()) rendered = true
      if (acquiresFrames) releaseFrames()
      still.isCompleted && (rendered || still.isCancelled)
    }
    still.await()
  }

  private fun releaseFrames() {
    while (true) {
      val frame = session.acquireFrame() ?: return
      frame.release(null)
    }
  }

  private fun requestForcedFrame(): ULong {
    val token = nextToken++
    session.requestFrame(FrameDemand(flags = FrameDemandFlag.FORCE, token = token))
    return token
  }

  private fun drainResults(): List<RenderFrameResult> = session.drainFrameResults()

  /**
   * Services driver work on the graphics thread, when the caller drives the session, and checks
   * [ready] after each round, blocking on the session's wakes, or on [completion] when it completes
   * off the driver, in between. A wake fires when a queue becomes nonempty, so work that arrives
   * after a round leaves a wake behind for the next receive.
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
            if (callerDriven) session.serviceDriverWork()
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
 * Runs [block] against a map with an attached owned-texture session of [width] x [height] on the
 * build's backend, and abandons and closes the session afterwards. The caller drives the session on
 * [TestGraphics.thread] unless [driver] names a core worker. The map's runtime is created from
 * [runtimeOptions], and [onFrameWake] runs on each frame wake.
 */
internal suspend fun <T> withOwnedTexture(
  width: Int = 32,
  height: Int = 16,
  mapMode: MapMode = MapMode.STATIC,
  driver: RenderDriverKind = RenderDriverKind.CALLER_GRAPHICS_THREAD,
  textureRingDepth: UInt = 1u,
  provider: ResourceProvider = denyingProvider(),
  runtimeOptions: RuntimeOptions = RuntimeOptions(),
  onFrameWake: () -> Unit = {},
  block: suspend RenderSessionFixture.() -> T,
): T =
  withMap(mapMode, provider, smallMapOptions(mapMode, width, height), runtimeOptions) {
    val canvas = TestGraphics.ownedTextureCanvas(width, height, driver)
    try {
      withRenderSession(
        driver,
        attach = { options ->
          TestGraphics.attachOwnedTexture(map, width, height, options, canvas)
        },
        textureRingDepth = textureRingDepth,
        onFrameWake = onFrameWake,
        block = block,
      )
    } finally {
      canvas?.close()
    }
  }

/**
 * Runs [block] against a session of this map that [attach] submits on [TestGraphics.thread] with
 * options for [driver], once the attach completes, and abandons and closes the session afterwards.
 */
internal suspend fun <T> MapFixture.withRenderSession(
  driver: RenderDriverKind,
  attach: suspend (RenderSessionAttachOptions) -> NewRenderSession,
  textureRingDepth: UInt = 1u,
  onFrameWake: () -> Unit = {},
  block: suspend RenderSessionFixture.() -> T,
): T {
  val wakes = Channel<Unit>(Channel.CONFLATED)
  val options =
    RenderSessionAttachOptions(
      driver = driver,
      requestedTextureRingDepth = textureRingDepth,
      frameWake =
        Wake {
          wakes.trySend(Unit)
          onFrameWake()
        },
      driverWorkWake = Wake { wakes.trySend(Unit) },
    )
  val attachment = withContext(TestGraphics.thread) { attach(options) }
  try {
    val fixture = RenderSessionFixture(this, attachment.session, wakes, driver)
    fixture.complete(attachment.ready, "the attach")
    return fixture.block()
  } finally {
    withContext(NonCancellable + TestGraphics.thread) { attachment.session.abandonAndClose() }
  }
}

/**
 * Leaves a session closed. An attached session is abandoned first, so its graphics objects are gone
 * before the fixture's device is, where a close alone would only schedule their destruction.
 */
internal fun RenderSessionHandle.abandonAndClose() {
  if (isClosed) return
  val state = getSnapshot().state
  if (state != RenderSessionState.DETACHED && state != RenderSessionState.ABANDONED) abandon()
  close()
}
