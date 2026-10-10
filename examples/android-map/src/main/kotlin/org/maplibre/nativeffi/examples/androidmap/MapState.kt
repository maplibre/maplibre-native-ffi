package org.maplibre.nativeffi.examples.androidmap

import kotlin.math.pow
import kotlin.math.round
import kotlinx.coroutines.runBlocking
import org.maplibre.nativeffi.generated.AnimationOptions
import org.maplibre.nativeffi.generated.CameraDelta
import org.maplibre.nativeffi.generated.CameraDeltaKind
import org.maplibre.nativeffi.generated.CameraOptions
import org.maplibre.nativeffi.generated.CameraUpdate
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.GesturePhase
import org.maplibre.nativeffi.generated.LatLng
import org.maplibre.nativeffi.generated.LogicalExtent
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.MapMode
import org.maplibre.nativeffi.generated.RuntimeEventMask
import org.maplibre.nativeffi.generated.RuntimeEventType
import org.maplibre.nativeffi.generated.ScreenPoint
import org.maplibre.nativeffi.generated.Wake

/**
 * The runtime and its map. Commands go straight to the runtime's own thread, and the runtime raises
 * [eventWake] when it has events to drain.
 */
internal class MapState(initialViewport: Viewport, eventWake: Wake, styleJson: String? = null) :
  AutoCloseable {
  private var closed = false
  private val initialCamera =
    CameraOptions(center = LatLng(37.7749, -122.4194), zoom = 13.0, bearing = 12.0, pitch = 30.0)

  private val runtime =
    GeneratedApi.runtimeCreate(
      GeneratedApi.runtimeOptionsDefault().copy(cachePath = ":memory:", eventWake = eventWake)
    )
  private lateinit var ownedMap: MapHandle
  val map: MapHandle
    get() = ownedMap

  init {
    try {
      ownedMap = runBlocking {
        runtime
          .createMap(
            GeneratedApi.mapOptionsDefault()
              .copy(
                initialExtent =
                  LogicalExtent(
                    initialViewport.logicalWidth.toUInt(),
                    initialViewport.logicalHeight.toUInt(),
                    initialViewport.scaleFactor,
                  ),
                mapMode = MapMode.CONTINUOUS,
                eventMask = RuntimeEventMask.MAP_RENDER_UPDATE_AVAILABLE,
              )
          )
          .await()
      }
      if (styleJson != null) map.setStyleJson(styleJson.encodeToByteArray())
      else map.setStyleUrl(STYLE_URL)
      map.updateCamera(CameraUpdate(camera = initialCamera))
    } catch (error: Throwable) {
      runBlocking {
        if (::ownedMap.isInitialized) ownedMap.release().await()
        runtime.release().await()
      }
      throw error
    }
  }

  fun cancelTransitions() {
    map.cancelTransitions()
  }

  fun setGestureInProgress(inProgress: Boolean) {
    map.updateCamera(
      CameraUpdate(gesturePhase = if (inProgress) GesturePhase.BEGIN else GesturePhase.END)
    )
  }

  fun moveBy(deltaX: Double, deltaY: Double) {
    map.applyCameraDelta(CameraDelta(offset = ScreenPoint(deltaX, deltaY)))
  }

  fun scaleBy(scale: Double, anchor: ScreenPoint) {
    map.applyCameraDelta(CameraDelta(kind = CameraDeltaKind.SCALE, amount = scale, anchor = anchor))
  }

  fun adjustBearing(degrees: Double, anchor: ScreenPoint) {
    map.applyCameraDelta(
      CameraDelta(kind = CameraDeltaKind.BEARING, amount = degrees, anchor = anchor)
    )
  }

  fun adjustPitch(degrees: Double) {
    map.applyCameraDelta(CameraDelta(kind = CameraDeltaKind.PITCH, amount = degrees))
  }

  /** Eases to the next whole zoom level, as `round(zoom) + 1`, about [anchor]. */
  fun zoomToNextWholeLevel(anchor: ScreenPoint) {
    val zoom = map.getCameraSnapshot().camera.zoom ?: 0.0
    map.applyCameraDelta(
      CameraDelta(
        kind = CameraDeltaKind.SCALE,
        amount = 2.0.pow(round(zoom) + 1.0 - zoom),
        anchor = anchor,
        animation = animation(DOUBLE_TAP_DURATION_MS),
      )
    )
  }

  fun resize(viewport: Viewport) {
    map.resize(
      LogicalExtent(
        viewport.logicalWidth.toUInt(),
        viewport.logicalHeight.toUInt(),
        viewport.scaleFactor,
      )
    )
  }

  private fun animation(durationMs: Double) = AnimationOptions(durationMs = durationMs)

  /** Drains every runtime event, and reports whether the map published an update to render. */
  fun drainRenderUpdates(): Boolean =
    runtime.drainEvents().use {
      it.get().events.any { event -> event.type == RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE }
    }

  override fun close() {
    if (closed) return
    closed = true
    runBlocking {
      try {
        map.release().await()
      } finally {
        runtime.release().await()
      }
    }
  }

  private companion object {
    private const val STYLE_URL = "https://tiles.openfreemap.org/styles/bright"
    private const val DOUBLE_TAP_DURATION_MS = 160.0
  }
}
