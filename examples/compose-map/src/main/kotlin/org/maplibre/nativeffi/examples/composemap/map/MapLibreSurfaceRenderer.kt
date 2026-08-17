package org.maplibre.nativeffi.examples.composemap.map

import java.util.concurrent.atomic.AtomicBoolean
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceFrame
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceRenderResult
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceRenderer
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceSession
import org.maplibre.nativeffi.examples.composemap.surface.ProducerBackend
import org.maplibre.nativeffi.examples.composemap.surface.SurfaceExtent
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.render.FrameDemand
import org.maplibre.nativeffi.render.RenderResult
import org.maplibre.nativeffi.render.RenderSessionHandle
import org.maplibre.nativeffi.runtime.OperationHandle

/**
 * The native-surface render loop.
 *
 * [render] runs on the bridge's producer thread, which owns the graphics context, borrowed texture,
 * and render session. Runtime and map updates are submitted directly to the core-owned worker.
 */
internal class MapLibreSurfaceRenderer : NativeSurfaceRenderer {
  override val backend: ProducerBackend = MapLibreNativeSurfaceAdapter.backend

  private val renderRequest = RenderRequest()
  private val closed = AtomicBoolean(false)

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
    state.drainNotifications()
    return try {
      renderAttached(state.map, frame)
    } catch (error: Throwable) {
      // The caller stops driving frames after this, so close the render session before the map.
      close()
      throw error
    }
  }

  private fun renderAttached(map: MapHandle, frame: NativeSurfaceFrame): NativeSurfaceRenderResult {
    val attached = ensureAttachedRenderSession(map, frame)

    if (!renderRequest.consume()) {
      return NativeSurfaceRenderResult.Skipped
    }
    attached.session.requestFrame(FrameDemand())
    attached.session.serviceDriverWork()
    if (attached.session.drainFrameResults().lastOrNull()?.disposition == RenderResult.RENDERED) {
      return NativeSurfaceRenderResult.Rendered
    }
    // A newly accepted map or target update may not have reached the render session yet.
    requestRender()
    return NativeSurfaceRenderResult.Skipped
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

  fun requestRender() {
    renderRequest.set()
    surfaceSession?.requestFrame()
  }

  private fun scheduleNotificationDrain() {
    surfaceSession?.requestFrame()
  }

  fun pan(deltaX: Double, deltaY: Double) {
    mapState?.moveBy(deltaX, deltaY)
    requestRender()
  }

  fun zoom(scale: Double, anchorX: Double, anchorY: Double) {
    mapState?.scaleBy(scale, ScreenPoint(anchorX, anchorY))
    requestRender()
  }

  fun animatePan(deltaX: Double, deltaY: Double) {
    mapState?.moveByAnimated(deltaX, deltaY)
    requestRender()
  }

  fun animateZoom(scale: Double) {
    mapState?.scaleByAnimated(scale, viewportCenter())
    requestRender()
  }

  fun rotateAndPitch(deltaX: Double, deltaY: Double) {
    mapState?.adjustBearingAndPitch(deltaX * DRAG_ROTATE_FACTOR, deltaY * DRAG_PITCH_FACTOR)
    requestRender()
  }

  fun animateBearing(deltaDegrees: Double) {
    mapState?.adjustBearingAnimated(deltaDegrees)
    requestRender()
  }

  fun animatePitch(deltaDegrees: Double) {
    mapState?.adjustPitchAnimated(deltaDegrees)
    requestRender()
  }

  fun resetOrientation() {
    mapState?.resetOrientation()
    requestRender()
  }

  fun stopCameraAnimation() {
    mapState?.cancelTransitions()
    requestRender()
  }

  fun setGestureActive(inProgress: Boolean) {
    mapState?.setGestureInProgress(inProgress)
    requestRender()
  }

  private fun <T> withRendererAccess(action: () -> T): T =
    ownerSession?.withRendererAccess(action) ?: action()

  private fun ensureMapState(extent: SurfaceExtent): MapState {
    mapState?.let {
      return it
    }
    return MapState(extent, ::scheduleNotificationDrain, ::requestRender).also { mapState = it }
  }

  private fun stopMapState() {
    val stopping = mapState
    mapState = null
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
          completeDriverOperation(existing.session, borrowed.setTarget(existing.session))
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
        renderRequest.set()
        return retargeted
      }
    }

    closeRenderSession()
    val attachment = borrowed.attach(map)
    try {
      completeDriverOperation(attachment.session, attachment.operation)
    } catch (error: Throwable) {
      runCatching { attachment.session.abandon() }
      runCatching { attachment.session.close() }
      throw error
    }
    val attached =
      AttachedRenderSession(borrowed.sessionKey, borrowed.targetKey, attachment.session)
    renderSession = attached
    renderRequest.set()
    return attached
  }

  private fun closeRenderSession() {
    val closing = renderSession
    renderSession = null
    closing?.session?.let { session ->
      completeDriverOperation(session, session.startDetach())
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

  private fun completeDriverOperation(session: RenderSessionHandle, operation: OperationHandle<*>) {
    operation.use {
      while (!it.poll()) session.serviceDriverWork()
      check(it.terminalStatus() == MaplibreStatus.OK) { it.diagnostic() }
    }
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
