package org.maplibre.nativeffi.examples.composemap.map

import java.util.concurrent.Semaphore
import java.util.concurrent.atomic.AtomicBoolean
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.runBlocking
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceFrame
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceRenderResult
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceRenderer
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceSession
import org.maplibre.nativeffi.examples.composemap.surface.ProducerBackend
import org.maplibre.nativeffi.examples.composemap.surface.SurfaceExtent
import org.maplibre.nativeffi.generated.FrameDemandFlag
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.RenderResult
import org.maplibre.nativeffi.generated.RenderSessionHandle
import org.maplibre.nativeffi.generated.ScreenPoint
import org.maplibre.nativeffi.generated.Wake

/**
 * The native-surface renderer.
 *
 * [render] runs on the bridge's producer thread, which owns the graphics context, borrowed texture,
 * and render session. It is the driver for the session: Skiko lends the texture only for the
 * frame's draw, so the session renders into it there, demanding a frame, servicing the driver, and
 * draining the result in one pass.
 *
 * Input becomes map commands on the Compose thread. Native wakes ask Compose for a draw: the
 * runtime's when a map update is ready to render, and the session's when it has driver work outside
 * a draw.
 */
internal class MapLibreSurfaceRenderer(
  private val styleJson: String? = null,
  private val onRendered: () -> Unit = {},
) : NativeSurfaceRenderer {
  override val backend: ProducerBackend = MapLibreNativeSurfaceAdapter.backend

  private val closed = AtomicBoolean(false)
  private val mapStateLock = Any()

  /** Set when the next draw should demand a frame: a map update or a repaint. */
  private val frameWanted = AtomicBoolean(false)

  /**
   * Set when the next draw should render even with no newer map update: for a new target, or to
   * retry a target that was not ready, which consumed the update.
   */
  private val frameForced = AtomicBoolean(false)

  /** Set by the runtime's event wake until the next draw drains the events. */
  private val eventsPending = AtomicBoolean(false)

  /** Set while a draw services the driver itself, so a driver-work wake needs no further draw. */
  private val servicing = AtomicBoolean(false)

  /** Released by the driver-work wake and by completions that teardown and attachment wait on. */
  private val driverWork = Semaphore(0)

  private val eventWake = Wake {
    eventsPending.set(true)
    surfaceSession?.requestFrame()
  }

  private val driverWorkWake = Wake {
    driverWork.release()
    if (!servicing.get()) surfaceSession?.requestFrame()
  }

  @Volatile private var surfaceSession: NativeSurfaceSession? = null
  @Volatile private var ownerSession: NativeSurfaceSession? = null
  @Volatile private var mapState: MapState? = null
  @Volatile private var renderSession: AttachedRenderSession? = null
  @Volatile private var currentExtent = SurfaceExtent.Empty

  override fun onSurfaceAvailable(session: NativeSurfaceSession) {
    surfaceSession = session
    ownerSession = session
    requestRender()
  }

  override fun onSurfaceChanged(extent: SurfaceExtent) {
    if (extent.isEmpty) {
      return
    }
    currentExtent = extent
    requestRender()
  }

  override fun render(frame: NativeSurfaceFrame): NativeSurfaceRenderResult {
    if (closed.get() || frame.extent.isEmpty) {
      return NativeSurfaceRenderResult.Skipped
    }

    val state = ensureMapState(frame.extent)
    state.resize(frame.extent)
    if (eventsPending.getAndSet(false) && state.drainRenderUpdates()) frameWanted.set(true)
    return try {
      renderAttached(state.map, frame)
    } catch (error: Throwable) {
      // The caller stops driving frames after this, so close the render session before the
      // map.
      close()
      throw error
    }
  }

  private fun renderAttached(map: MapHandle, frame: NativeSurfaceFrame): NativeSurfaceRenderResult {
    val session = ensureAttachedRenderSession(map, frame).session
    servicing.set(true)
    try {
      val forced = frameForced.getAndSet(false)
      if (frameWanted.getAndSet(false) || forced) {
        val demand = GeneratedApi.frameDemandDefault()
        session.requestFrame(if (forced) demand.copy(flags = FrameDemandFlag(0u)) else demand)
      }
      session.serviceDriverWork(0uL)
    } finally {
      servicing.set(false)
    }
    // Driver work that arrived before the flag cleared raised no draw, so it runs now.
    session.serviceDriverWork(0uL)
    val batch =
      try {
        session.drainFrameResults()
      } catch (error: MaplibreException) {
        if (error.status != MaplibreStatus.NOT_READY) throw error
        return NativeSurfaceRenderResult.Skipped
      }
    var rendered = false
    batch.use { results ->
      for (index in 0uL until results.count()) {
        val result = results.get(index)
        when (result.disposition) {
          RenderResult.RENDERED -> rendered = true
          // The next Compose frame retries the frame.
          RenderResult.TARGET_NOT_READY -> requestRender(force = true)
        }
        // The result carries the map's own follow-up demand, so an ongoing transition needs no
        // runtime event round trip.
        if (result.needsRepaint) requestRender()
      }
    }
    if (!rendered) return NativeSurfaceRenderResult.Skipped
    onRendered()
    return NativeSurfaceRenderResult.Rendered
  }

  override fun onSurfaceLost() {
    withRendererAccess { closeRenderSession() }
    surfaceSession = null
  }

  override fun close() {
    if (!closed.compareAndSet(false, true)) {
      return
    }
    surfaceSession = null
    val owner = ownerSession
    ownerSession = null
    // A map with an attached session cannot be destroyed, so the session closes first.
    if (renderSession != null && owner != null) {
      owner.withRendererAccess {
        closeRenderSession()
        stopMapState()
      }
    } else if (renderSession != null) {
      abandonRenderSession()
      stopMapState()
    } else {
      stopMapState()
    }
  }

  /**
   * Asks Compose for a draw that demands a frame. A forced frame renders even when the map has no
   * newer update.
   */
  fun requestRender(force: Boolean = false) {
    (if (force) frameForced else frameWanted).set(true)
    surfaceSession?.requestFrame()
  }

  fun moveBy(deltaX: Double, deltaY: Double) {
    updateMap { it.moveBy(deltaX, deltaY) }
  }

  fun scaleBy(scale: Double, anchorX: Double, anchorY: Double) {
    updateMap { it.scaleBy(scale, ScreenPoint(anchorX, anchorY)) }
  }

  fun moveByAnimated(deltaX: Double, deltaY: Double) {
    updateMap { it.moveByAnimated(deltaX, deltaY) }
  }

  fun scaleByAnimated(scale: Double) {
    updateMap { it.scaleByAnimated(scale, viewportCenter()) }
  }

  fun rotateAndPitchBy(deltaX: Double, deltaY: Double) {
    updateMap { it.adjustBearingAndPitch(deltaX * DRAG_ROTATE_FACTOR, deltaY * DRAG_PITCH_FACTOR) }
  }

  fun rotateBy(deltaDegrees: Double) {
    updateMap { it.adjustBearingAnimated(deltaDegrees) }
  }

  fun pitchBy(deltaDegrees: Double) {
    updateMap { it.adjustPitchAnimated(deltaDegrees) }
  }

  fun resetOrientation() {
    updateMap { it.resetOrientation() }
  }

  fun cancelTransitions() {
    updateMap { it.cancelTransitions() }
  }

  fun setGestureInProgress(inProgress: Boolean) {
    updateMap { it.setGestureInProgress(inProgress) }
  }

  /**
   * Submits one camera command against the live map. The producer thread closes the map under the
   * same monitor, so a handler on the Compose thread sees a live map or none at all. The map's
   * update reaches the next draw through the event wake.
   */
  private fun updateMap(action: (MapState) -> Unit) {
    synchronized(mapStateLock) {
      val state = mapState ?: return
      action(state)
    }
  }

  private fun <T> withRendererAccess(action: () -> T): T =
    ownerSession?.withRendererAccess(action) ?: action()

  private fun ensureMapState(extent: SurfaceExtent): MapState {
    mapState?.let {
      return it
    }
    return MapState(extent, eventWake, styleJson).also {
      synchronized(mapStateLock) { mapState = it }
    }
  }

  private fun stopMapState() {
    val stopping = synchronized(mapStateLock) { mapState.also { mapState = null } }
    stopping?.close()
  }

  /**
   * Renders through a session attached to the texture this frame carries. Skiko reallocates its
   * texture on every resize; handing the replacement to the live session keeps its renderer warm,
   * so a session is closed and reattached only when the graphics context itself changes.
   */
  private fun ensureAttachedRenderSession(
    map: MapHandle,
    frame: NativeSurfaceFrame,
  ): AttachedRenderSession {
    val borrowed = MapLibreNativeSurfaceAdapter.borrowedTarget(frame.target, frame.extent)
    renderSession?.let { existing ->
      if (existing.sessionKey == borrowed.sessionKey) {
        if (existing.targetKey == borrowed.targetKey) {
          return existing
        }
        try {
          awaitDriverWork(existing.session, borrowed.setTarget(existing.session))
        } catch (error: RuntimeException) {
          // A failed handover leaves it unknown which texture the session holds, and Skiko frees
          // the outgoing one as soon as it moves on, so close the session.
          try {
            closeRenderSession()
          } catch (cleanupError: Exception) {
            error.addSuppressed(cleanupError)
          }
          throw error
        }
        val retargeted = existing.copy(targetKey = borrowed.targetKey)
        renderSession = retargeted
        // Handing over a texture publishes no map update, so the new texture needs a forced frame.
        frameForced.set(true)
        return retargeted
      }
    }

    closeRenderSession()
    val attachment = borrowed.attach(map, driverWorkWake)
    try {
      awaitDriverWork(attachment.session, attachment.ready)
    } catch (error: Throwable) {
      runCatching { attachment.session.abandon() }
      runCatching { attachment.session.close() }
      throw error
    }
    val attached =
      AttachedRenderSession(borrowed.sessionKey, borrowed.targetKey, attachment.session)
    renderSession = attached
    frameForced.set(true)
    return attached
  }

  private fun closeRenderSession() {
    val closing = renderSession
    renderSession = null
    closing?.session?.let { session ->
      awaitDriverWork(session, session.detach())
      session.close()
    }
  }

  private fun abandonRenderSession() {
    val closing = renderSession
    renderSession = null
    closing?.session?.let { session ->
      session.abandon()
      session.close()
    }
  }

  /**
   * Services driver work on the producer thread until [completion] finishes, sleeping until the
   * driver-work wake or the completion arrives.
   */
  private fun awaitDriverWork(session: RenderSessionHandle, completion: Deferred<Unit>) {
    // Permits from wakes that draws already serviced would only spin the loop.
    driverWork.drainPermits()
    completion.invokeOnCompletion { driverWork.release() }
    servicing.set(true)
    try {
      while (true) {
        session.serviceDriverWork(0uL)
        if (completion.isCompleted) break
        driverWork.acquire()
      }
    } finally {
      servicing.set(false)
    }
    runBlocking { completion.await() }
  }

  private fun viewportCenter(): ScreenPoint {
    val extent = currentExtent
    return ScreenPoint(extent.width / 2.0, extent.height / 2.0)
  }

  private data class AttachedRenderSession(
    val sessionKey: MapLibreNativeSurfaceAdapter.SessionKey,
    val targetKey: MapLibreNativeSurfaceAdapter.TargetKey,
    val session: RenderSessionHandle,
  )

  private companion object {
    private const val DRAG_ROTATE_FACTOR = 0.5
    private const val DRAG_PITCH_FACTOR = 0.5
  }
}
