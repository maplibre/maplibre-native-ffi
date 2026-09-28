package org.maplibre.nativeffi.examples.lwjglmap

import java.util.concurrent.atomic.AtomicBoolean
import kotlinx.coroutines.runBlocking
import org.maplibre.nativeffi.generated.AnimationOptions
import org.maplibre.nativeffi.generated.CameraDelta
import org.maplibre.nativeffi.generated.CameraDeltaKind
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
    update(CameraOptions(bearing = 0.0, pitch = 0.0), durationMs)
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
    AnimationOptions(durationMs = durationMs)

  private fun drainEvents(): Boolean {
    val batch =
      try {
        runtime.drainEvents()
      } catch (error: org.maplibre.nativeffi.error.MaplibreException) {
        if (error.status == org.maplibre.nativeffi.error.MaplibreStatus.NOT_READY) return false
        throw error
      }
    return batch.use { owner ->
      owner.get().events.any { event -> event.type == RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE }
    }
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

    fun create(viewport: Viewport): MapState {
      val runtime =
        GeneratedApi.runtimeCreate(
          GeneratedApi.runtimeOptionsDefault().copy(cachePath = ":memory:")
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
        map.setStyleUrl(STYLE_URL)
        map.updateCamera(CameraUpdate(camera = initialCamera))
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

/** One-bit signal that a frame is worth drawing. */
internal class RenderRequest {
  private val requested = AtomicBoolean(true)

  fun set() {
    requested.set(true)
  }

  fun consume(): Boolean = requested.getAndSet(false)
}
