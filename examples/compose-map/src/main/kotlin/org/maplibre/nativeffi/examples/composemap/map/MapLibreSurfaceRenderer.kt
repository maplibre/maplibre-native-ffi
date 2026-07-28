package org.maplibre.nativeffi.examples.composemap.map

import java.util.concurrent.atomic.AtomicBoolean
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceFrame
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceRenderResult
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceRenderer
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceSession
import org.maplibre.nativeffi.examples.composemap.surface.ProducerBackend
import org.maplibre.nativeffi.examples.composemap.surface.SurfaceExtent
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.render.RenderSessionHandle

/**
 * The render loop.
 *
 * [render] runs on the bridge's producer thread, which owns the host graphics context and the
 * borrowed texture, so that thread attaches the render session, renders through it, reattaches it
 * on resize, and closes it. It touches the map only to attach, which native serves from any thread.
 *
 * Input decoding runs on the Compose thread and only enqueues camera commands; [MapRuntimeLoop]
 * applies them on the thread that owns the map.
 */
internal class MapLibreSurfaceRenderer : NativeSurfaceRenderer {
  override val backend: ProducerBackend = MapLibreNativeSurfaceAdapter.backend

  private val commands = CameraCommandQueue()
  private val renderRequest = RenderRequest()
  private val closed = AtomicBoolean(false)
  private val failureReported = AtomicBoolean(false)

  @Volatile private var surfaceSession: NativeSurfaceSession? = null
  @Volatile private var ownerSession: NativeSurfaceSession? = null
  @Volatile private var runtimeLoop: MapRuntimeLoop? = null
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

    val loop = ensureRuntimeLoop(frame.extent)
    loop.failure?.let { error ->
      if (failureReported.compareAndSet(false, true)) {
        throw IllegalStateException("map runtime loop failed", error)
      }
      return NativeSurfaceRenderResult.Skipped
    }
    val map = loop.map ?: return NativeSurfaceRenderResult.Skipped
    val attached = ensureAttachedRenderSession(map, frame)

    // Consume before rendering, so a request the runtime loop publishes during the render call is
    // not discarded.
    if (!renderRequest.consume()) {
      return NativeSurfaceRenderResult.Skipped
    }
    if (attached.session.renderUpdate()) {
      return NativeSurfaceRenderResult.Rendered
    }
    // The map applies a new logical size on the runtime loop's next pump, so an attach or resize
    // is followed by frames with nothing to render. Keep pacing and retry.
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
    // The render loop closes the session before the runtime loop closes the map: a map with an
    // attached session cannot be destroyed.
    if (renderSession != null && owner != null) {
      owner.withRendererAccess {
        closeRenderSession()
        stopRuntimeLoop()
      }
    } else {
      stopRuntimeLoop()
    }
  }

  fun requestRender() {
    renderRequest.set()
    surfaceSession?.requestFrame()
  }

  fun moveBy(deltaX: Double, deltaY: Double) {
    enqueue(CameraCommand.MoveBy(deltaX, deltaY))
  }

  fun scaleBy(scale: Double, anchorX: Double, anchorY: Double) {
    enqueue(CameraCommand.ScaleBy(scale, ScreenPoint(anchorX, anchorY)))
  }

  fun moveByAnimated(deltaX: Double, deltaY: Double) {
    enqueue(CameraCommand.MoveByAnimated(deltaX, deltaY))
  }

  fun scaleByAnimated(scale: Double) {
    enqueue(CameraCommand.ScaleByAnimated(scale, viewportCenter()))
  }

  fun rotateAndPitchBy(deltaX: Double, deltaY: Double) {
    enqueue(
      CameraCommand.AdjustBearingAndPitch(deltaX * DRAG_ROTATE_FACTOR, -deltaY * DRAG_PITCH_FACTOR)
    )
  }

  fun rotateBy(deltaDegrees: Double) {
    enqueue(CameraCommand.AdjustBearingAnimated(deltaDegrees))
  }

  fun pitchBy(deltaDegrees: Double) {
    enqueue(CameraCommand.AdjustPitchAnimated(deltaDegrees))
  }

  fun resetPitchAndBearing() {
    enqueue(CameraCommand.ResetOrientation)
  }

  fun cancelTransitions() {
    enqueue(CameraCommand.CancelTransitions)
  }

  private fun enqueue(command: CameraCommand) {
    commands.enqueue(command)
    requestRender()
  }

  private fun <T> withRendererAccess(action: () -> T): T =
    ownerSession?.withRendererAccess(action) ?: action()

  private fun ensureRuntimeLoop(extent: SurfaceExtent): MapRuntimeLoop {
    runtimeLoop?.let { existing ->
      if (existing.scaleFactor == extent.scaleFactor) {
        return existing
      }
      closeRenderSession()
      stopRuntimeLoop()
    }
    return MapRuntimeLoop(extent, commands, renderRequest).also { runtimeLoop = it }
  }

  private fun stopRuntimeLoop() {
    val stopping = runtimeLoop
    runtimeLoop = null
    stopping?.close()
  }

  private fun ensureAttachedRenderSession(
    map: MapHandle,
    frame: NativeSurfaceFrame,
  ): AttachedRenderSession {
    val descriptor = MapLibreNativeSurfaceAdapter.descriptor(frame.target, frame.extent)
    renderSession?.let { existing ->
      if (existing.key == descriptor.key) {
        return existing
      }
    }

    closeRenderSession()
    val attached = AttachedRenderSession(descriptor.key, descriptor.attach(map))
    renderSession = attached
    renderRequest.set()
    return attached
  }

  private fun closeRenderSession() {
    val closing = renderSession
    renderSession = null
    closing?.session?.close()
  }

  private fun viewportCenter(): ScreenPoint {
    val extent = currentExtent
    return ScreenPoint(extent.width / 2.0, extent.height / 2.0)
  }

  private data class AttachedRenderSession(
    val key: MapLibreNativeSurfaceAdapter.TargetKey,
    val session: RenderSessionHandle,
  )

  private companion object {
    private const val DRAG_ROTATE_FACTOR = 0.5
    private const val DRAG_PITCH_FACTOR = 0.5
  }
}
