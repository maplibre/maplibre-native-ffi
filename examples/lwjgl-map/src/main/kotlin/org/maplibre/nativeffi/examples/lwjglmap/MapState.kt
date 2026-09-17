package org.maplibre.nativeffi.examples.lwjglmap

import java.util.concurrent.atomic.AtomicBoolean
import kotlinx.coroutines.runBlocking
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.CameraDelta
import org.maplibre.nativeffi.camera.CameraDeltaKind
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.CameraUpdate
import org.maplibre.nativeffi.camera.CameraUpdateMode
import org.maplibre.nativeffi.camera.GesturePhase
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.runtime.RuntimeEventMask
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

/** Runtime and map state driven by the core-owned runtime worker. */
internal class MapState
private constructor(private val runtime: RuntimeHandle, val map: MapHandle) : AutoCloseable {

  fun cancelTransitions() {
    map.cancelTransitions()
  }

  fun setGestureInProgress(inProgress: Boolean) {
    map.updateCamera(
      CameraUpdate(gesturePhase = if (inProgress) GesturePhase.BEGIN else GesturePhase.END)
    )
  }

  fun moveBy(dx: Double, dy: Double, durationMs: Double? = null) {
    map.applyCameraDelta(
      CameraDelta(offset = ScreenPoint(dx, dy), animation = animation(durationMs))
    )
  }

  fun scaleBy(scale: Double, anchor: ScreenPoint, durationMs: Double? = null) {
    map.applyCameraDelta(
      CameraDelta(
        kind = CameraDeltaKind.SCALE,
        amount = scale,
        anchor = anchor,
        animation = animation(durationMs),
      )
    )
  }

  fun adjustPitch(delta: Double, durationMs: Double? = null) {
    map.applyCameraDelta(
      CameraDelta(kind = CameraDeltaKind.PITCH, amount = delta, animation = animation(durationMs))
    )
  }

  fun adjustBearing(delta: Double, durationMs: Double? = null) {
    map.applyCameraDelta(
      CameraDelta(kind = CameraDeltaKind.BEARING, amount = delta, animation = animation(durationMs))
    )
  }

  fun resetOrientation(durationMs: Double) {
    update(
      CameraOptions().apply {
        bearing = 0.0
        pitch = 0.0
      },
      durationMs,
    )
  }

  /** Drains the runtime event stream during the host's paced loop turn. */
  fun pollEvents(renderRequest: RenderRequest) {
    if (drainEvents()) renderRequest.set()
  }

  private fun update(camera: CameraOptions, durationMs: Double? = null) {
    map.updateCamera(
      CameraUpdate(
        mode = if (durationMs == null) CameraUpdateMode.JUMP else CameraUpdateMode.EASE,
        camera = camera,
        animation = animation(durationMs),
      )
    )
  }

  private fun animation(durationMs: Double?): AnimationOptions =
    AnimationOptions().apply { durationMs?.let { this.durationMs = it } }

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
    runBlocking {
      try {
        map.close().await()
      } finally {
        runtime.close().await()
      }
    }
  }

  companion object {
    private const val STYLE_URL = "https://tiles.openfreemap.org/styles/bright"

    fun create(viewport: Viewport): MapState {
      val runtime = RuntimeHandle.create(RuntimeOptions().apply { cachePath = ":memory:" })
      val initialCamera =
        CameraOptions().apply {
          center = LatLng(37.7749, -122.4194)
          zoom = 13.0
          bearing = 12.0
          pitch = 30.0
        }
      val map =
        try {
          runBlocking {
            MapHandle.create(
                runtime,
                MapOptions().apply {
                  width = viewport.width()
                  height = viewport.height()
                  scaleFactor = viewport.scaleFactor()
                  mapMode = MapMode.CONTINUOUS
                  eventMask = RuntimeEventMask.MAP_RENDER_UPDATE_AVAILABLE
                },
              )
              .await()
          }
        } catch (error: Throwable) {
          runBlocking { runtime.close().await() }
          throw error
        }
      try {
        val state = MapState(runtime, map)
        map.setStyleUrl(STYLE_URL)
        map.updateCamera(CameraUpdate(camera = initialCamera))
        return state
      } catch (error: Throwable) {
        runBlocking {
          map.close().await()
          runtime.close().await()
        }
        throw error
      }
    }
  }
}

/** One-bit signal that a frame is worth drawing. */
internal class RenderRequest {
  private val requested = AtomicBoolean(true)

  fun set() {
    requested.set(true)
  }

  fun consume(): Boolean = requested.getAndSet(false)
}
