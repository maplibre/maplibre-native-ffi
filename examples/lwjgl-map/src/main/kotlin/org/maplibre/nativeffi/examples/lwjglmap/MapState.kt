package org.maplibre.nativeffi.examples.lwjglmap

import java.util.concurrent.atomic.AtomicBoolean
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.CameraDelta
import org.maplibre.nativeffi.camera.CameraDeltaKind
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.CameraUpdate
import org.maplibre.nativeffi.camera.CameraUpdateMode
import org.maplibre.nativeffi.camera.GesturePhase
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.map.MapSize
import org.maplibre.nativeffi.runtime.ReadyEndpoint
import org.maplibre.nativeffi.runtime.RuntimeEventMask
import org.maplibre.nativeffi.runtime.RuntimeEventPayload
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

/** Runtime and map state driven by the core-owned runtime worker. */
internal class MapState
private constructor(
  private val runtime: RuntimeHandle,
  val map: MapHandle,
  private val scheduleNotificationDrain: () -> Unit,
) : AutoCloseable {
  private val notificationPending = AtomicBoolean(false)

  init {
    runtime.setNotificationCallback {
      if (notificationPending.compareAndSet(false, true)) {
        scheduleNotificationDrain()
      }
    }
  }

  fun cancelTransitions() {
    map.updateCamera(CameraUpdate())
  }

  fun setGestureInProgress(inProgress: Boolean) {
    map.updateCamera(
      CameraUpdate(gesturePhase = if (inProgress) GesturePhase.BEGIN else GesturePhase.END)
    )
  }

  fun moveBy(dx: Double, dy: Double, durationMs: Double? = null) {
    map.applyCameraDelta(
      CameraDelta(
        offset = org.maplibre.nativeffi.geo.ScreenPoint(dx, dy),
        animation = animation(durationMs) ?: AnimationOptions(),
      )
    )
  }

  fun scaleBy(
    scale: Double,
    anchor: org.maplibre.nativeffi.geo.ScreenPoint,
    durationMs: Double? = null,
  ) {
    map.applyCameraDelta(
      CameraDelta(
        kind = CameraDeltaKind.SCALE,
        amount = scale,
        anchor = anchor,
        animation = animation(durationMs) ?: AnimationOptions(),
      )
    )
  }

  fun adjustPitch(delta: Double, durationMs: Double? = null) {
    map.applyCameraDelta(
      CameraDelta(
        kind = CameraDeltaKind.PITCH,
        amount = delta,
        animation = animation(durationMs) ?: AnimationOptions(),
      )
    )
  }

  fun adjustBearing(delta: Double, durationMs: Double? = null) {
    map.applyCameraDelta(
      CameraDelta(
        kind = CameraDeltaKind.BEARING,
        amount = delta,
        animation = animation(durationMs) ?: AnimationOptions(),
      )
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

  fun resize(viewport: Viewport) {
    map.resize(MapSize(viewport.width(), viewport.height(), viewport.scaleFactor()))
  }

  fun requestRepaint() {
    map.requestRepaint()
  }

  /** Drains owned readiness and event batches on the GLFW receiver thread. */
  fun drainNotifications(renderRequest: RenderRequest) {
    if (!notificationPending.getAndSet(false)) return
    do {
      val ready = runtime.drainReady()
      if (ready.any { it.kind == ReadyEndpoint.Kind.DRIVER_WORK }) {
        renderRequest.set()
      }
      if (ready.any { it.kind == ReadyEndpoint.Kind.RUNTIME_EVENTS } && drainEvents()) {
        renderRequest.set()
      }
    } while (notificationPending.getAndSet(false))
  }

  private fun update(camera: CameraOptions, durationMs: Double? = null) {
    map.updateCamera(
      CameraUpdate(
        mode = if (durationMs == null) CameraUpdateMode.JUMP else CameraUpdateMode.EASE,
        camera = camera,
        animation = AnimationOptions().apply { durationMs?.let { this.durationMs = it } },
      )
    )
  }

  private fun animation(durationMs: Double?): AnimationOptions? = durationMs?.let { duration ->
    AnimationOptions().apply { this.durationMs = duration }
  }

  private fun drainEvents(): Boolean {
    var renderUpdateAvailable = false
    for (event in runtime.drainEvents().events) {
      if (event.mapSource != map) continue
      if (event.type == RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE) {
        renderUpdateAvailable = true
      } else if (
        event.type == RuntimeEventType.MAP_RENDER_FRAME_FINISHED &&
          (event.payload as? RuntimeEventPayload.RenderFrame)?.needsRepaint == true
      ) {
        renderUpdateAvailable = true
      }
    }
    return renderUpdateAvailable
  }

  override fun close() {
    runtime.clearNotificationCallback()
    try {
      runSuspend { map.close() }
    } finally {
      runSuspend { runtime.close() }
    }
  }

  companion object {
    private const val STYLE_URL = "https://tiles.openfreemap.org/styles/bright"

    fun create(viewport: Viewport, scheduleNotificationDrain: () -> Unit): MapState {
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
          runSuspend {
            MapHandle.create(
              runtime,
              MapOptions().apply {
                width = viewport.width()
                height = viewport.height()
                scaleFactor = viewport.scaleFactor()
                mapMode = MapMode.CONTINUOUS
                eventMask =
                  RuntimeEventMask.MAP_RENDER_UPDATE_AVAILABLE +
                    RuntimeEventMask.MAP_RENDER_FRAME_FINISHED
              },
            )
          }
        } catch (error: Throwable) {
          runSuspend { runtime.close() }
          throw error
        }
      try {
        val state = MapState(runtime, map, scheduleNotificationDrain)
        map.setStyleUrl(STYLE_URL)
        map.updateCamera(CameraUpdate(camera = initialCamera))
        return state
      } catch (error: Throwable) {
        runSuspend { map.close() }
        runSuspend { runtime.close() }
        throw error
      }
    }
  }
}

/** A one-bit frame request shared with notification callbacks. */
internal class RenderRequest {
  private val requested = AtomicBoolean(true)

  fun set() {
    requested.set(true)
  }

  fun consume(): Boolean = requested.getAndSet(false)
}
