package org.maplibre.nativeffi.examples.composemap.map

import java.util.concurrent.Semaphore
import java.util.concurrent.atomic.AtomicBoolean
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.runBlocking
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceFrame
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceRenderResult
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceRenderer
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceSession
import org.maplibre.nativeffi.examples.composemap.surface.ProducerBackend
import org.maplibre.nativeffi.examples.composemap.surface.SurfaceExtent
import org.maplibre.nativeffi.examples.composemap.surface.VulkanDeviceRetention
import org.maplibre.nativeffi.generated.AcquiredFrameHandle
import org.maplibre.nativeffi.generated.FrameDemand
import org.maplibre.nativeffi.generated.FrameDemandFlag
import org.maplibre.nativeffi.generated.GpuSyncKind
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.RenderDriverKind
import org.maplibre.nativeffi.generated.RenderFrameResult
import org.maplibre.nativeffi.generated.RenderResult
import org.maplibre.nativeffi.generated.RenderSessionAttachOptions
import org.maplibre.nativeffi.generated.RenderSessionHandle
import org.maplibre.nativeffi.generated.ScreenPoint
import org.maplibre.nativeffi.generated.Wake

/**
 * The native-surface renderer.
 *
 * [render] runs inside the bridge's producer access, the only window in which the bridge lends its
 * texture ring. There it demands a frame and waits for that demand's result. A Metal or Vulkan
 * session renders on its core worker meanwhile; an OpenGL session's caller driver runs on the
 * producer thread, which services it while it waits.
 *
 * Each rendered frame is acquired and handed to the bridge, which draws it after [render] returns.
 * The renderer holds that frame until a newer one is acquired, so the session never renders into
 * the texture the bridge draws: every bridge lends a ring of two, and the session renders into the
 * other texture meanwhile.
 *
 * Input becomes map commands on the Compose thread. A waiting frame demand renders each update, and
 * its frame wake asks Compose to draw the result.
 */
internal class MapLibreSurfaceRenderer(
  private val styleJson: String? = null,
  private val onRendered: () -> Unit = {},
) : NativeSurfaceRenderer {
  override val backend: ProducerBackend = MapLibreNativeSurfaceAdapter.backend

  private val driver = MapLibreNativeSurfaceAdapter.driver
  private val callerDriver = driver == RenderDriverKind.CALLER_GRAPHICS_THREAD

  private val closed = AtomicBoolean(false)
  private val mapStateLock = Any()

  /** Set when the next draw should demand a frame: a map update or a repaint. */
  private val frameWanted = AtomicBoolean(false)

  /**
   * Set when the next draw should render even with no newer map update: for a new target, or to
   * retry a target that was not ready.
   */
  private val frameForced = AtomicBoolean(false)

  /** Set while a draw waits on the session, so a driver-work wake needs no further draw. */
  private val drawing = AtomicBoolean(false)

  /** Released by the session's wakes and by the completions that a draw waits on. */
  private val sessionWork = Semaphore(0)

  private val frameWake = Wake {
    sessionWork.release()
    surfaceSession?.requestFrame()
  }

  /** Caller-driver work that arrives outside a draw asks for one, which services it. */
  private val driverWorkWake = Wake {
    sessionWork.release()
    if (!drawing.get()) surfaceSession?.requestFrame()
  }

  @Volatile private var surfaceSession: NativeSurfaceSession? = null
  @Volatile private var ownerSession: NativeSurfaceSession? = null
  @Volatile private var mapState: MapState? = null
  @Volatile private var renderSession: AttachedRenderSession? = null
  @Volatile private var currentExtent = SurfaceExtent.Empty
  private var nextToken = 0uL

  /** The frame the bridge draws, held until it is replaced or its ring is. */
  private var held: AcquiredFrameHandle? = null

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
    drawing.set(true)
    val result =
      try {
        renderAttached(state.map, frame)
      } catch (error: Throwable) {
        drawing.set(false)
        // The caller stops driving frames after this, so close the render session before the
        // map.
        close()
        throw error
      }
    drawing.set(false)
    // Driver work that arrived before the flag cleared asked for no draw, so it runs now.
    if (callerDriver) renderSession?.session?.serviceDriverWork()
    return result
  }

  private fun renderAttached(map: MapHandle, frame: NativeSurfaceFrame): NativeSurfaceRenderResult {
    val session = ensureAttachedRenderSession(map, frame).session
    if (callerDriver) session.serviceDriverWork()
    val forced = frameForced.getAndSet(false)
    val wanted = frameWanted.getAndSet(false)
    val ready =
      session.drainFrameResults()?.use {
        val results = it.get().results
        results.lastOrNull { result -> result.disposition == RenderResult.RENDERED }
          ?: results.lastOrNull { result -> result.disposition == RenderResult.TARGET_NOT_READY }
      }
    val result =
      if (forced) {
        // Release the previous frame before another forced render needs its slot.
        if (ready?.disposition == RenderResult.RENDERED) acquireNewest(session)
        val token = ++nextToken
        session.requestFrame(
          FrameDemand(
            flags = FrameDemandFlag(0u),
            token = token,
          ),
        )
        awaitResult(session, token)
      } else if (ready != null) {
        ready
      } else if (wanted) {
        val token = ++nextToken
        session.requestFrame(FrameDemand(flags = FrameDemandFlag.IF_NEEDED, token = token))
        awaitResult(session, token)
      } else {
        return NativeSurfaceRenderResult.Skipped
      }
    return when (result.disposition) {
      RenderResult.RENDERED -> {
        val slot = acquireNewest(session)
        if (slot == null) {
          // A frame rendered before a replacement can no longer be acquired; the next Compose
          // frame renders into the new ring.
          requestRender(force = true)
          return NativeSurfaceRenderResult.Skipped
        }
        session.requestFrame(
          FrameDemand(
            flags = FrameDemandFlag.IF_NEEDED or FrameDemandFlag.WAIT_FOR_UPDATE,
            token = ++nextToken,
          ),
        )
        onRendered()
        NativeSurfaceRenderResult.Rendered(frame.target.slot(slot))
      }
      RenderResult.TARGET_NOT_READY -> {
        // The next Compose frame retries a target that was not ready. The retry keeps the force,
        // because the demand that failed may have been forced.
        requestRender(force = true)
        NativeSurfaceRenderResult.Skipped
      }
      else -> NativeSurfaceRenderResult.Skipped
    }
  }

  /**
   * Holds the newest rendered frame, releasing every older one, and returns its ring slot, or null
   * when no frame is ready. The bridge finished with the frame this replaces before this call.
   */
  private fun acquireNewest(session: RenderSessionHandle): Int? {
    var newest: AcquiredFrameHandle? = null
    while (true) {
      val frame = session.acquireFrame() ?: break
      // Nothing read an older frame, so it releases CPU-complete.
      newest?.release()
      newest = frame
    }
    val frame = newest ?: return null
    frame.withProducerSync { sync ->
      check(sync.kind == GpuSyncKind.CPU_COMPLETE) {
        "The bridges draw only frames whose producer work is complete"
      }
    }
    releaseHeld()
    held = frame
    return MapLibreNativeSurfaceAdapter.frameSlot(frame)
  }

  /**
   * Releases the held frame. CPU-complete release states that the bridge is done with it, which
   * holds from the next render on.
   */
  private fun releaseHeld() {
    val releasing = held ?: return
    held = null
    releasing.release()
  }

  /**
   * Waits for the result of the demand with [token], draining every result meanwhile. A caller
   * driver renders only when serviced, so the producer thread services it between waits.
   */
  private fun awaitResult(session: RenderSessionHandle, token: ULong): RenderFrameResult {
    while (true) {
      if (callerDriver) session.serviceDriverWork()
      val result =
        session.drainFrameResults()?.use { results ->
          results.get().results.lastOrNull { it.token == token }
        }
      if (result != null) return result
      sessionWork.acquire()
    }
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
   * waiting demand renders the update and wakes the next draw.
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
    return MapState(extent, styleJson).also {
      synchronized(mapStateLock) { mapState = it }
    }
  }

  private fun stopMapState() {
    val stopping = synchronized(mapStateLock) { mapState.also { mapState = null } }
    stopping?.close()
  }

  /**
   * Renders through a session attached to the ring this frame carries. The bridge reallocates its
   * ring on every resize; handing the replacement to the live session keeps its renderer warm, so a
   * session is closed and reattached only when the graphics context, ring depth, or scale changes.
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
          // A replacement is refused while the host holds a frame of the ring.
          releaseHeld()
          await(existing.session, borrowed.setTarget(existing.session))
        } catch (error: RuntimeException) {
          // A failed replacement leaves it unknown which texture the session holds, and Skiko
          // frees the outgoing one as soon as it moves on, so close the session.
          try {
            closeRenderSession()
          } catch (cleanupError: Exception) {
            error.addSuppressed(cleanupError)
          }
          throw error
        }
        val retargeted = existing.copy(targetKey = borrowed.targetKey)
        renderSession = retargeted
        // Demand a frame for the replacement texture.
        frameForced.set(true)
        return retargeted
      }
    }

    closeRenderSession()
    val attachment =
      borrowed.attach(
        map,
        RenderSessionAttachOptions(
          driver = driver,
          frameWake = frameWake,
          driverWorkWake = if (callerDriver) driverWorkWake else Wake(),
        ),
      )
    try {
      await(attachment.session, attachment.ready)
    } catch (error: Throwable) {
      runCatching { abandon(attachment.session) }
      runCatching { attachment.session.close() }
      throw error
    }
    val attached =
      AttachedRenderSession(borrowed.sessionKey, borrowed.targetKey, attachment.session)
    renderSession = attached
    frameForced.set(true)
    return attached
  }

  /** Detaches and destroys the session. A failed detach abandons it. */
  private fun closeRenderSession() {
    val closing = renderSession ?: return
    renderSession = null
    val session = closing.session
    try {
      releaseHeld()
      await(session, session.detach())
    } catch (error: RuntimeException) {
      System.err.println("render session detach failed, abandoning: ${error.message}")
      abandon(session)
    } finally {
      session.close()
    }
  }

  private fun abandonRenderSession() {
    val closing = renderSession
    renderSession = null
    runCatching { releaseHeld() }
    closing?.session?.let { session ->
      abandon(session)
      session.close()
    }
  }

  /**
   * Ends the session's graphics work at once. Graphics objects that the abandon kept are children
   * of the producer's Vulkan device, which then stays until the process exits.
   */
  private fun abandon(session: RenderSessionHandle) {
    val abandoned = session.abandon()
    if (abandoned.quarantinedResourceCount > 0u) {
      VulkanDeviceRetention.keepUntilExit = true
      System.err.println(
        "render session abandon kept ${abandoned.quarantinedResourceCount} resource groups until exit",
      )
    }
  }

  /**
   * Waits for a session operation on the producer thread. A caller driver's operation progresses
   * only through driver service, so the thread services it between waits for a wake or for the
   * completion.
   */
  private fun <T> await(session: RenderSessionHandle, completion: Deferred<T>): T {
    completion.invokeOnCompletion { sessionWork.release() }
    while (true) {
      if (callerDriver) session.serviceDriverWork()
      if (completion.isCompleted) break
      sessionWork.acquire()
    }
    return runBlocking { completion.await() }
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
