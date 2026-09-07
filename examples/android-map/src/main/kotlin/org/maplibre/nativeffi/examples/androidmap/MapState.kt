package org.maplibre.nativeffi.examples.androidmap

import java.util.concurrent.atomic.AtomicBoolean
import kotlin.math.pow
import kotlin.math.round
import kotlinx.coroutines.runBlocking
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.CameraDelta
import org.maplibre.nativeffi.camera.CameraDeltaKind
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.CameraUpdate
import org.maplibre.nativeffi.camera.GesturePhase
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.map.MapSize
import org.maplibre.nativeffi.runtime.RuntimeEventMask
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

/** Runtime and map state driven by the core-owned runtime worker. */
internal class MapState(initialViewport: Viewport, private val startLoop: () -> Unit) :
  AutoCloseable {
  private var closed = false
  private val initialCamera =
    CameraOptions().apply {
      center = LatLng(37.7749, -122.4194)
      zoom = 13.0
      bearing = 12.0
      pitch = 30.0
    }

  private val runtime = RuntimeHandle.create(RuntimeOptions().apply { cachePath = ":memory:" })
  private lateinit var ownedMap: MapHandle
  val map: MapHandle
    get() = ownedMap

  val renderRequest = RenderRequest()

  init {
    try {
      ownedMap = runBlocking {
        MapHandle.create(
            runtime,
            MapOptions().apply {
              width = initialViewport.logicalWidth
              height = initialViewport.logicalHeight
              scaleFactor = initialViewport.scaleFactor
              mapMode = MapMode.CONTINUOUS
              eventMask = RuntimeEventMask.MAP_RENDER_UPDATE_AVAILABLE
            },
          )
          .await()
      }
      map.setStyleUrl(STYLE_URL)
      map.updateCamera(CameraUpdate(camera = initialCamera))
    } catch (error: Throwable) {
      runBlocking {
        if (::ownedMap.isInitialized) ownedMap.close().await()
        runtime.close().await()
      }
      throw error
    }
  }

  fun cancelTransitions() {
    map.cancelTransitions()
    requestRender()
  }

  fun setGestureInProgress(inProgress: Boolean) {
    map.updateCamera(
      CameraUpdate(gesturePhase = if (inProgress) GesturePhase.BEGIN else GesturePhase.END)
    )
    requestRender()
  }

  fun moveBy(deltaX: Double, deltaY: Double) {
    map.applyCameraDelta(CameraDelta(offset = ScreenPoint(deltaX, deltaY)))
    requestRender()
  }

  fun scaleBy(scale: Double, anchor: ScreenPoint) {
    map.applyCameraDelta(CameraDelta(kind = CameraDeltaKind.SCALE, amount = scale, anchor = anchor))
    requestRender()
  }

  fun adjustBearing(degrees: Double, anchor: ScreenPoint) {
    map.applyCameraDelta(
      CameraDelta(kind = CameraDeltaKind.BEARING, amount = degrees, anchor = anchor)
    )
    requestRender()
  }

  fun adjustPitch(degrees: Double) {
    map.applyCameraDelta(CameraDelta(kind = CameraDeltaKind.PITCH, amount = degrees))
    requestRender()
  }

  /** Eases to the next whole zoom level, as `round(zoom) + 1`, about [anchor]. */
  fun zoomToNextWholeLevel(anchor: ScreenPoint) {
    val zoom = map.cameraSnapshot().camera.zoom ?: 0.0
    map.applyCameraDelta(
      CameraDelta(
        kind = CameraDeltaKind.SCALE,
        amount = 2.0.pow(round(zoom) + 1.0 - zoom),
        anchor = anchor,
        animation = animation(DOUBLE_TAP_DURATION_MS),
      )
    )
    requestRender()
  }

  fun resize(viewport: Viewport) {
    map.resize(MapSize(viewport.logicalWidth, viewport.logicalHeight, viewport.scaleFactor))
  }

  fun requestRepaint() {
    map.requestRepaint()
  }

  /** Marks a frame worth drawing and starts the view's paced loop if it is idle. */
  fun requestRender() {
    renderRequest.set()
    startLoop()
  }

  fun pollEvents() {
    if (drainEvents()) renderRequest.set()
  }

  private fun animation(durationMs: Double) =
    AnimationOptions().apply { this.durationMs = durationMs }

  private fun drainEvents(): Boolean {
    var renderUpdateAvailable = false
    for (event in runtime.drainEvents()) {
      if (event.mapSource != map) continue
      if (event.type == RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE) {
        renderUpdateAvailable = true
      }
    }
    return renderUpdateAvailable
  }

  override fun close() {
    if (closed) return
    closed = true
    runBlocking {
      try {
        map.close().await()
      } finally {
        runtime.close().await()
      }
    }
  }

  private companion object {
    private const val STYLE_URL = "https://tiles.openfreemap.org/styles/bright"
    private const val DOUBLE_TAP_DURATION_MS = 160.0
  }
}

/** One-bit signal that a frame is worth drawing. */
internal class RenderRequest {
  private val value = AtomicBoolean(true)

  fun set() {
    value.set(true)
  }

  fun consume(): Boolean = value.getAndSet(false)
}
