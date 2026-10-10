package org.maplibre.nativeffi.examples.lwjglmap

import kotlinx.coroutines.runBlocking
import org.maplibre.nativeffi.generated.AnimationOptions
import org.maplibre.nativeffi.generated.CameraDelta
import org.maplibre.nativeffi.generated.CameraOptions
import org.maplibre.nativeffi.generated.CameraUpdate
import org.maplibre.nativeffi.generated.CameraUpdateMode
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.GesturePhase
import org.maplibre.nativeffi.generated.LatLng
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.MapMode
import org.maplibre.nativeffi.generated.RuntimeEventMask
import org.maplibre.nativeffi.generated.RuntimeEventType
import org.maplibre.nativeffi.generated.RuntimeHandle
import org.maplibre.nativeffi.generated.ScreenPoint
import org.maplibre.nativeffi.generated.Wake

/** The runtime and its map. Commands go straight to the runtime's own thread. */
internal class MapState
private constructor(private val runtime: RuntimeHandle, val map: MapHandle) : AutoCloseable {

  fun cancelTransitions() {
    map.cancelTransitions().reportFailure("camera transition cancel")
  }

  fun setGestureInProgress(inProgress: Boolean) {
    map
      .updateCamera(
        CameraUpdate(gesturePhase = if (inProgress) GesturePhase.BEGIN else GesturePhase.END)
      )
      .reportFailure("gesture update")
  }

  fun moveBy(dx: Double, dy: Double, durationMs: Double? = null) {
    delta(CameraDelta(offset = ScreenPoint(dx, dy), animation = animation(durationMs)))
  }

  fun scaleBy(scale: Double, anchor: ScreenPoint, durationMs: Double? = null) {
    delta(CameraDelta(scale = scale, anchor = anchor, animation = animation(durationMs)))
  }

  fun adjustOrientation(
    bearing: Double? = null,
    pitch: Double? = null,
    durationMs: Double? = null,
  ) {
    delta(CameraDelta(bearing = bearing, pitch = pitch, animation = animation(durationMs)))
  }

  fun resetOrientation(durationMs: Double) {
    update(CameraOptions(bearing = 0.0, pitch = 0.0), durationMs)
  }

  private fun update(camera: CameraOptions, durationMs: Double? = null) {
    map
      .updateCamera(
        CameraUpdate(
          mode = if (durationMs == null) CameraUpdateMode.JUMP else CameraUpdateMode.EASE,
          camera = camera,
          animation = animation(durationMs),
        )
      )
      .reportFailure("camera update")
  }

  private fun delta(delta: CameraDelta) {
    map.applyCameraDelta(delta).reportFailure("camera delta")
  }

  private fun animation(durationMs: Double?): AnimationOptions =
    AnimationOptions(durationMs = durationMs)

  /** Drains every runtime event, and reports whether the map published an update to render. */
  fun drainRenderUpdates(): Boolean =
    runtime.drainEvents().use { owner ->
      owner.get().events.any { event -> event.type == RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE }
    }

  override fun close() {
    runBlocking {
      try {
        map.release().await()
      } finally {
        runtime.release().await()
      }
    }
  }

  companion object {
    private const val STYLE_URL = "https://tiles.openfreemap.org/styles/bright"

    /** An inline style the smoke run renders, which requests no resources. */
    const val SMOKE_STYLE =
      """{"version":8,"sources":{},"layers":[{"id":"background","type":"background","paint":{"background-color":"#2a6f97"}}]}"""

    /**
     * Creates the runtime and map, loading [styleJson] when given and the default URL otherwise.
     * The runtime raises [eventWake] when it has events to drain.
     */
    fun create(viewport: Viewport, eventWake: Wake, styleJson: String? = null): MapState {
      val runtime =
        GeneratedApi.runtimeCreate(
          GeneratedApi.runtimeOptionsDefault().copy(cachePath = ":memory:", eventWake = eventWake)
        )
      val initialCamera =
        CameraOptions(
          center = LatLng(37.7749, -122.4194),
          zoom = 13.0,
          bearing = 12.0,
          pitch = 30.0,
        )
      val map =
        try {
          runBlocking {
            runtime
              .mapCreate(
                GeneratedApi.mapOptionsDefault()
                  .copy(
                    initialExtent =
                      GeneratedApi.mapOptionsDefault()
                        .initialExtent
                        .copy(
                          width = (viewport.width()).toUInt(),
                          height = (viewport.height()).toUInt(),
                          scaleFactor = viewport.scaleFactor(),
                        ),
                    mapMode = MapMode.CONTINUOUS,
                    eventMask = RuntimeEventMask.MAP_RENDER_UPDATE_AVAILABLE,
                  )
              )
              .await()
          }
        } catch (error: Throwable) {
          runBlocking { runtime.release().await() }
          throw error
        }
      try {
        val state = MapState(runtime, map)
        val style =
          if (styleJson != null) map.setStyleJson(styleJson.encodeToByteArray())
          else map.setStyleUrl(STYLE_URL)
        style.reportFailure("style load")
        map.updateCamera(CameraUpdate(camera = initialCamera)).reportFailure("initial camera")
        return state
      } catch (error: Throwable) {
        runBlocking {
          map.release().await()
          runtime.release().await()
        }
        throw error
      }
    }
  }
}
