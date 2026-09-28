package org.maplibre.nativeffi.render

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.generated.AcquiredFrameHandle
import org.maplibre.nativeffi.generated.FrameDemand
import org.maplibre.nativeffi.generated.FrameDemandFlag
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.MapMode
import org.maplibre.nativeffi.generated.RenderDriverKind
import org.maplibre.nativeffi.generated.RenderFrameResult
import org.maplibre.nativeffi.generated.RenderResult
import org.maplibre.nativeffi.generated.RenderSessionAttachOptions
import org.maplibre.nativeffi.generated.RenderSessionAttachment
import org.maplibre.nativeffi.generated.RenderSessionCapabilityFlag
import org.maplibre.nativeffi.generated.RenderSessionHandle
import org.maplibre.nativeffi.generated.RenderSessionState
import org.maplibre.nativeffi.generated.RuntimeHandle
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
 * The fixture must initialize the selected backend; unavailable drivers fail the test.
 */
internal expect object OwnedTextureTestSupport {
  fun attach(
    map: MapHandle,
    width: Int,
    height: Int,
    textureRingDepth: UInt,
  ): OwnedTextureTestSession
}

/** Attach options every fixture uses: the fixture's context is current on the calling thread. */
internal val OWNED_TEXTURE_ATTACH_OPTIONS: RenderSessionAttachOptions =
  RenderSessionAttachOptions(
    driver = RenderDriverKind.CALLER_GRAPHICS_THREAD,
    requestedTextureRingDepth = 1u,
  )

/**
 * Completes [deferred] while servicing the driver work only this thread can run.
 *
 * The loop is bounded so a completion nothing resolves fails the test instead of hanging the suite.
 */
internal suspend fun <T> RenderSessionHandle.completeOnDriver(
  deferred: Deferred<T>,
  attempts: Int = DRIVER_SERVICE_ATTEMPTS,
  service: RenderSessionHandle.() -> Unit = { serviceDriverWork(0uL) },
): T {
  repeat(attempts) {
    if (deferred.isCompleted) return deferred.await()
    service()
  }
  error("a driver-serviced completion never resolved; session state: ${getSnapshot().state}")
}

/** Demands one frame and services driver work until the demand reaches a terminal result. */
internal fun RenderSessionHandle.renderOneFrame(
  demand: FrameDemand = GeneratedApi.frameDemandDefault().copy(flags = FrameDemandFlag(0u)),
  attempts: Int = DRIVER_SERVICE_ATTEMPTS,
): RenderFrameResult {
  requestFrame(demand)
  repeat(attempts) {
    serviceDriverWork(0uL)
    val results = drainTestFrameResults()
    if (results.isNotEmpty()) return results.last()
  }
  error("a frame demand never reached a terminal result; session state: ${getSnapshot().state}")
}

/** Service rounds a bounded driver loop runs before it calls the work stuck. */
private const val DRIVER_SERVICE_ATTEMPTS: Int = 100_000

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
  val present =
    (getCapabilities().flags.rawValue and RenderSessionCapabilityFlag.PRESENTATION.rawValue) != 0u
  for (attempt in 0 until attempts) {
    val token = getSnapshot().latestDemandToken + 1uL
    requestFrame(
      GeneratedApi.frameDemandDefault()
        .copy(flags = if (present) FrameDemandFlag.PRESENT else FrameDemandFlag(0u), token = token)
    )
    for (poll in 0 until 10) {
      val result = drainTestFrameResults().lastOrNull { it.token == token }
      if (result != null) {
        if (result.disposition == RenderResult.RENDERED) return result
        last = result
        break
      }
      sleepMillis(1)
    }
  }
  error("the session never rendered a frame; last frame result: $last")
}

/** Leaves a session closable: an attached session is abandoned rather than leaked. */
internal fun RenderSessionHandle.abandonAndClose() {
  if (isClosed) return
  val state = getSnapshot().state
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
  textureRingDepth: UInt = 1u,
  block: suspend (RuntimeHandle, MapHandle, OwnedTextureTestSession) -> Unit,
) {
  val failures = mutableListOf<Throwable>()
  val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
  val map =
    runtime
      .mapCreate(
        GeneratedApi.mapOptionsDefault()
          .copy(
            initialExtent =
              GeneratedApi.mapOptionsDefault()
                .initialExtent
                .copy(width = (mapWidth).toUInt(), height = (mapHeight).toUInt()),
            mapMode = mapMode,
          )
      )
      .await()
  try {
    val owned = OwnedTextureTestSupport.attach(map, width, height, textureRingDepth)
    try {
      owned.session.completeOnDriver(owned.attachment.ready)
      block(runtime, map, owned)
    } catch (error: Throwable) {
      failures += error
    }
    failures.addIfFailed { owned.close() }
    failures.addIfFailed { runtime.barrier().await() }
  } catch (error: Throwable) {
    failures += error
  }
  failures.addIfFailed { map.release().await() }
  failures.addIfFailed { runtime.release().await() }
  failures.firstOrNull()?.let { throw it }
}

private inline fun MutableList<Throwable>.addIfFailed(block: () -> Unit) {
  runCatching(block).exceptionOrNull()?.let { add(it) }
}

/** Copies the drained batch while its native owner is live. */
internal fun RenderSessionHandle.drainTestFrameResults(): List<RenderFrameResult> {
  val batch =
    try {
      drainFrameResults()
    } catch (error: org.maplibre.nativeffi.error.MaplibreException) {
      if (error.status == org.maplibre.nativeffi.error.MaplibreStatus.NOT_READY) return emptyList()
      throw error
    }
  return batch.use { owner -> List(owner.count().toInt()) { owner.get(it.toULong()) } }
}

internal fun attachOwnedTextureFixture(
  map: MapHandle,
  width: Int,
  height: Int,
  textureRingDepth: UInt,
  attach: (MapHandle, Int, Int, RenderSessionAttachOptions) -> RenderSessionAttachment,
  frameSize: (AcquiredFrameHandle) -> OwnedTextureFrameSize,
  releaseGraphics: () -> Unit,
): OwnedTextureTestSession {
  val attachment =
    try {
      attach(
        map,
        width,
        height,
        OWNED_TEXTURE_ATTACH_OPTIONS.copy(requestedTextureRingDepth = textureRingDepth),
      )
    } catch (error: Throwable) {
      releaseGraphics()
      throw error
    }
  return object : OwnedTextureTestSession {
    override val attachment = attachment

    override fun attachAnotherOwnedTexture(width: Int, height: Int) =
      attach(map, width, height, OWNED_TEXTURE_ATTACH_OPTIONS)

    override fun frameSize(frame: AcquiredFrameHandle) = frameSize(frame)

    override fun close() {
      try {
        session.abandonAndClose()
      } finally {
        releaseGraphics()
      }
    }
  }
}
