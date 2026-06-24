package org.maplibre.nativeffi.examples.composemap.map

import kotlin.math.max
import kotlin.math.min
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceFrame
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceRenderResult
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceRenderer
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceSession
import org.maplibre.nativeffi.examples.composemap.surface.ProducerBackend
import org.maplibre.nativeffi.examples.composemap.surface.SurfaceExtent
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.render.RenderSessionHandle
import org.maplibre.nativeffi.runtime.RuntimeEventPayload
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

internal class MapLibreSurfaceRenderer : NativeSurfaceRenderer {
  override val backend: ProducerBackend = MapLibreNativeSurfaceAdapter.backend

  private var surfaceSession: NativeSurfaceSession? = null
  private var ownerSession: NativeSurfaceSession? = null
  private var runtime: RuntimeHandle? = null
  private var map: MapHandle? = null
  private var mapScaleFactor: Double? = null
  private var renderSession: AttachedRenderSession? = null
  private var currentExtent = SurfaceExtent.Empty
  private var renderPending = true
  private var closed = false

  override fun onSurfaceAvailable(session: NativeSurfaceSession) {
    session.withRendererAccess {
      check(!closed) { "renderer is closed" }
      surfaceSession = session
      ownerSession = session
      renderPending = true
      session.requestFrame()
    }
  }

  override fun onSurfaceChanged(extent: SurfaceExtent) {
    withRendererAccess {
      if (closed || extent.isEmpty) {
        return@withRendererAccess
      }
      currentExtent = extent
      renderPending = true
      surfaceSession?.requestFrame()
    }
  }

  override fun render(frame: NativeSurfaceFrame): NativeSurfaceRenderResult {
    check(!closed) { "renderer is closed" }
    if (frame.extent.isEmpty) {
      return NativeSurfaceRenderResult.Skipped
    }

    val currentMap = ensureMap(frame.extent)
    val currentRuntime = checkNotNull(runtime) { "runtime was not created" }

    currentRuntime.runOnce()
    drainEvents(currentRuntime, currentMap)

    val attached = ensureAttachedRenderSession(currentMap, frame)
    if (!renderPending) {
      return NativeSurfaceRenderResult.Skipped
    }

    return try {
      attached.session.renderUpdate()
      renderPending = false
      NativeSurfaceRenderResult.Rendered
    } catch (_: InvalidStateException) {
      renderPending = true
      surfaceSession?.requestFrame()
      NativeSurfaceRenderResult.Skipped
    }
  }

  override fun onSurfaceLost() {
    withRendererAccess {
      surfaceSession = null
      closeRenderSession()
    }
  }

  fun requestRender() {
    withRendererAccess {
      if (closed) {
        return@withRendererAccess
      }
      renderPending = true
      surfaceSession?.requestFrame()
    }
  }

  fun moveBy(deltaX: Double, deltaY: Double) {
    withRendererAccess {
      map?.moveBy(deltaX, deltaY)
      requestRender()
    }
  }

  fun scaleBy(scale: Double, anchorX: Double, anchorY: Double) {
    withRendererAccess {
      map?.scaleBy(scale, ScreenPoint(anchorX, anchorY))
      requestRender()
    }
  }

  fun moveByAnimated(deltaX: Double, deltaY: Double) {
    withRendererAccess {
      map?.moveByAnimated(deltaX, deltaY, KEYBOARD_ANIMATION)
      requestRender()
    }
  }

  fun scaleByAnimated(scale: Double) {
    withRendererAccess {
      map?.scaleByAnimated(scale, viewportCenter(), KEYBOARD_ANIMATION)
      requestRender()
    }
  }

  fun rotateAndPitchBy(deltaX: Double, deltaY: Double) {
    withRendererAccess {
      val currentMap = map ?: return@withRendererAccess
      val camera = currentMap.camera
      currentMap.jumpTo(
        CameraOptions().apply {
          bearing = (camera.bearing ?: 0.0) + deltaX * DRAG_ROTATE_FACTOR
          pitch = ((camera.pitch ?: 0.0) - deltaY * DRAG_PITCH_FACTOR).clampPitch()
        }
      )
      requestRender()
    }
  }

  fun rotateBy(deltaDegrees: Double) {
    withRendererAccess {
      val currentMap = map ?: return@withRendererAccess
      currentMap.easeTo(
        CameraOptions().apply { bearing = (currentMap.camera.bearing ?: 0.0) + deltaDegrees },
        KEYBOARD_ANIMATION,
      )
      requestRender()
    }
  }

  fun pitchBy(deltaDegrees: Double) {
    withRendererAccess {
      val currentMap = map ?: return@withRendererAccess
      currentMap.easeTo(
        CameraOptions().apply {
          pitch = ((currentMap.camera.pitch ?: 0.0) + deltaDegrees).clampPitch()
        },
        KEYBOARD_ANIMATION,
      )
      requestRender()
    }
  }

  fun resetPitchAndBearing() {
    withRendererAccess {
      map?.easeTo(
        CameraOptions().apply {
          bearing = 0.0
          pitch = 0.0
        },
        RESET_ANIMATION,
      )
      requestRender()
    }
  }

  fun cancelTransitions() {
    withRendererAccess { map?.cancelTransitions() }
  }

  override fun close() {
    withRendererAccess { closeOwnedResources() }
  }

  private fun closeOwnedResources() {
    if (closed) {
      return
    }
    closed = true
    surfaceSession = null
    val closingMap = map
    val closingRuntime = runtime
    map = null
    runtime = null
    mapScaleFactor = null
    ownerSession = null
    try {
      closeRenderSession()
    } finally {
      try {
        closingMap?.close()
      } finally {
        closingRuntime?.close()
      }
    }
  }

  private fun <T> withRendererAccess(action: () -> T): T =
    ownerSession?.withRendererAccess(action) ?: action()

  private fun ensureMap(extent: SurfaceExtent): MapHandle {
    map?.let { existing ->
      if (mapScaleFactor == extent.scaleFactor) {
        return existing
      }
      closeMapForScaleFactorChange()
    }

    val createdRuntime = RuntimeHandle.create(RuntimeOptions().apply { cachePath = ":memory:" })
    val createdMap =
      try {
        MapHandle.create(
          createdRuntime,
          MapOptions().apply {
            width = extent.width
            height = extent.height
            scaleFactor = extent.scaleFactor
            mapMode = MapMode.CONTINUOUS
          },
        )
      } catch (error: RuntimeException) {
        createdRuntime.close()
        throw error
      }
    try {
      createdMap.setStyleUrl(STYLE_URL)
      createdMap.jumpTo(
        CameraOptions().apply {
          center = LatLng(37.7749, -122.4194)
          zoom = 13.0
          bearing = 12.0
          pitch = 30.0
        }
      )
      runtime = createdRuntime
      map = createdMap
      mapScaleFactor = extent.scaleFactor
      return createdMap
    } catch (error: RuntimeException) {
      try {
        createdMap.close()
      } finally {
        createdRuntime.close()
      }
      throw error
    }
  }

  private fun closeMapForScaleFactorChange() {
    val closingMap = map
    val closingRuntime = runtime
    map = null
    runtime = null
    mapScaleFactor = null
    renderPending = true
    try {
      closeRenderSession()
    } finally {
      try {
        closingMap?.close()
      } finally {
        closingRuntime?.close()
      }
    }
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
    renderPending = true
    return attached
  }

  private fun drainEvents(runtime: RuntimeHandle, map: MapHandle) {
    while (true) {
      val event = runtime.pollEvent() ?: return
      if (event.mapSource != map) {
        continue
      }
      if (event.type == RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE) {
        renderPending = true
      } else if (
        event.type == RuntimeEventType.MAP_RENDER_FRAME_FINISHED &&
          (event.payload as? RuntimeEventPayload.RenderFrame)?.needsRepaint == true
      ) {
        renderPending = true
      }
    }
  }

  private fun closeRenderSession() {
    val closing = renderSession
    renderSession = null
    closing?.session?.close()
  }

  private fun viewportCenter(): ScreenPoint =
    ScreenPoint(currentExtent.width / 2.0, currentExtent.height / 2.0)

  private data class AttachedRenderSession(
    val key: MapLibreNativeSurfaceAdapter.TargetKey,
    val session: RenderSessionHandle,
  )

  private companion object {
    private const val STYLE_URL = "https://tiles.openfreemap.org/styles/bright"
    private const val DRAG_ROTATE_FACTOR = 0.5
    private const val DRAG_PITCH_FACTOR = 0.5
    private val KEYBOARD_ANIMATION = AnimationOptions().apply { durationMs = 160.0 }
    private val RESET_ANIMATION = AnimationOptions().apply { durationMs = 160.0 }
  }
}

private fun Double.clampPitch(): Double = max(0.0, min(60.0, this))
