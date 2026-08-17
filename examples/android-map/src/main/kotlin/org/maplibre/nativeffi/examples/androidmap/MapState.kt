package org.maplibre.nativeffi.examples.androidmap

import android.os.Handler
import android.os.Looper
import java.util.concurrent.atomic.AtomicBoolean
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.CameraDelta
import org.maplibre.nativeffi.camera.CameraDeltaKind
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.CameraUpdate
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
internal class MapState(initialViewport: Viewport, private val requestRender: () -> Unit) :
  AutoCloseable {
  private val receiver = Handler(Looper.getMainLooper())
  private val notificationPending = AtomicBoolean(false)
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

  private val drainNotifications = Runnable {
    if (!closed && notificationPending.getAndSet(false)) {
      do {
        val ready = runtime.drainReady()
        if (ready.any { it.kind == ReadyEndpoint.Kind.DRIVER_WORK }) {
          renderRequest.set()
          requestRender()
        }
        if (ready.any { it.kind == ReadyEndpoint.Kind.RUNTIME_EVENTS } && drainEvents()) {
          renderRequest.set()
          requestRender()
        }
      } while (notificationPending.getAndSet(false))
    }
  }

  init {
    runtime.setNotificationCallback {
      if (notificationPending.compareAndSet(false, true)) {
        receiver.post(drainNotifications)
      }
    }
    try {
      ownedMap = runSuspend {
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = initialViewport.logicalWidth
            height = initialViewport.logicalHeight
            scaleFactor = initialViewport.scaleFactor
            mapMode = MapMode.CONTINUOUS
            eventMask =
              RuntimeEventMask.MAP_RENDER_UPDATE_AVAILABLE +
                RuntimeEventMask.MAP_RENDER_FRAME_FINISHED
          },
        )
      }
      map.setStyleUrl(STYLE_URL)
      map.updateCamera(CameraUpdate(camera = initialCamera))
    } catch (error: Throwable) {
      runtime.clearNotificationCallback()
      if (::ownedMap.isInitialized) runSuspend { ownedMap.close() }
      runSuspend { runtime.close() }
      throw error
    }
  }

  fun cancelTransitions() {
    map.updateCamera(CameraUpdate())
    requestRender()
  }

  fun setGestureInProgress(inProgress: Boolean) {
    map.updateCamera(
      CameraUpdate(gesturePhase = if (inProgress) GesturePhase.BEGIN else GesturePhase.END)
    )
    requestRender()
  }

  fun moveBy(deltaX: Double, deltaY: Double) {
    map.applyCameraDelta(
      CameraDelta(offset = org.maplibre.nativeffi.geo.ScreenPoint(deltaX, deltaY))
    )
    requestRender()
  }

  fun scaleBy(scale: Double, anchor: org.maplibre.nativeffi.geo.ScreenPoint) {
    map.applyCameraDelta(CameraDelta(kind = CameraDeltaKind.SCALE, amount = scale, anchor = anchor))
    requestRender()
  }

  fun adjustBearing(degrees: Double, anchor: org.maplibre.nativeffi.geo.ScreenPoint) {
    map.applyCameraDelta(
      CameraDelta(kind = CameraDeltaKind.BEARING, amount = degrees, anchor = anchor)
    )
    requestRender()
  }

  fun adjustPitch(degrees: Double) {
    map.applyCameraDelta(CameraDelta(kind = CameraDeltaKind.PITCH, amount = degrees))
    requestRender()
  }

  fun zoomToNextWholeLevel(anchor: org.maplibre.nativeffi.geo.ScreenPoint) {
    map.applyCameraDelta(
      CameraDelta(
        kind = CameraDeltaKind.SCALE,
        amount = 2.0,
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

  private fun animation(durationMs: Double) =
    AnimationOptions().apply { this.durationMs = durationMs }

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
    if (closed) return
    closed = true
    receiver.removeCallbacks(drainNotifications)
    runtime.clearNotificationCallback()
    try {
      runSuspend { map.close() }
    } finally {
      runSuspend { runtime.close() }
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
