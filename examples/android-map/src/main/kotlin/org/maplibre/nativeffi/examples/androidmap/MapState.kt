package org.maplibre.nativeffi.examples.androidmap

import java.util.concurrent.atomic.AtomicBoolean
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

/** Runtime and map state driven by the core-owned runtime worker. */
internal class MapState(initialViewport: Viewport, private val startLoop: () -> Unit) :
  AutoCloseable {
  private var closed = false
  private val initialCamera =
    CameraOptions(center = LatLng(37.7749, -122.4194), zoom = 13.0, bearing = 12.0, pitch = 30.0)

  private val runtime =
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault().copy(cachePath = ":memory:"))
  private lateinit var ownedMap: MapHandle
  val map: MapHandle
    get() = ownedMap

  val renderRequest = RenderRequest()

  init {
    try {
      ownedMap = runBlocking {
        runtime
          .mapCreate(
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
      map.setStyleUrl(STYLE_URL)
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
    val zoom = map.cameraSnapshotGet().camera.zoom ?: 0.0
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
    map.resize(
      LogicalExtent(
        viewport.logicalWidth.toUInt(),
        viewport.logicalHeight.toUInt(),
        viewport.scaleFactor,
      )
    )
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

  private fun animation(durationMs: Double) = AnimationOptions(durationMs = durationMs)

  private fun drainEvents(): Boolean {
    val batch =
      try {
        runtime.drainEvents()
      } catch (error: org.maplibre.nativeffi.error.MaplibreException) {
        if (error.status == org.maplibre.nativeffi.error.MaplibreStatus.NOT_READY) return false
        throw error
      }
    return batch.use {
      it.get().events.any { event -> event.type == RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE }
    }
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

/** One-bit signal that a frame is worth drawing. */
internal class RenderRequest {
  private val value = AtomicBoolean(true)

  fun set() {
    value.set(true)
  }

  fun consume(): Boolean = value.getAndSet(false)
}
