package org.maplibre.nativeffi.examples.composemap.map

import java.util.concurrent.atomic.AtomicBoolean
import kotlinx.coroutines.runBlocking
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.CameraDelta
import org.maplibre.nativeffi.camera.CameraDeltaKind
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.CameraUpdate
import org.maplibre.nativeffi.camera.CameraUpdateMode
import org.maplibre.nativeffi.camera.GesturePhase
import org.maplibre.nativeffi.examples.composemap.surface.SurfaceExtent
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
internal class MapState(initialExtent: SurfaceExtent, private val requestRender: () -> Unit) :
  AutoCloseable {
  private var closed = false
  private var currentSize =
    MapSize(initialExtent.width, initialExtent.height, initialExtent.scaleFactor)
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

  init {
    try {
      ownedMap = runBlocking {
        MapHandle.create(
            runtime,
            MapOptions().apply {
              width = initialExtent.width
              height = initialExtent.height
              scaleFactor = initialExtent.scaleFactor
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
  }

  fun setGestureInProgress(inProgress: Boolean) {
    map.updateCamera(
      CameraUpdate(gesturePhase = if (inProgress) GesturePhase.BEGIN else GesturePhase.END)
    )
  }

  fun moveBy(deltaX: Double, deltaY: Double) {
    map.applyCameraDelta(CameraDelta(offset = ScreenPoint(deltaX, deltaY)))
  }

  fun moveByAnimated(deltaX: Double, deltaY: Double) {
    map.applyCameraDelta(
      CameraDelta(
        offset = ScreenPoint(deltaX, deltaY),
        animation = animation(KEYBOARD_ANIMATION_MS),
      )
    )
  }

  fun scaleBy(scale: Double, anchor: ScreenPoint) {
    map.applyCameraDelta(CameraDelta(kind = CameraDeltaKind.SCALE, amount = scale, anchor = anchor))
  }

  fun scaleByAnimated(scale: Double, anchor: ScreenPoint) {
    map.applyCameraDelta(
      CameraDelta(
        kind = CameraDeltaKind.SCALE,
        amount = scale,
        anchor = anchor,
        animation = animation(KEYBOARD_ANIMATION_MS),
      )
    )
  }

  fun adjustBearingAndPitch(bearingDegrees: Double, pitchDegrees: Double) {
    map.applyCameraDelta(CameraDelta(kind = CameraDeltaKind.BEARING, amount = bearingDegrees))
    map.applyCameraDelta(CameraDelta(kind = CameraDeltaKind.PITCH, amount = pitchDegrees))
  }

  fun adjustBearingAnimated(bearingDegrees: Double) {
    map.applyCameraDelta(
      CameraDelta(
        kind = CameraDeltaKind.BEARING,
        amount = bearingDegrees,
        animation = animation(KEYBOARD_ANIMATION_MS),
      )
    )
  }

  fun adjustPitchAnimated(pitchDegrees: Double) {
    map.applyCameraDelta(
      CameraDelta(
        kind = CameraDeltaKind.PITCH,
        amount = pitchDegrees,
        animation = animation(KEYBOARD_ANIMATION_MS),
      )
    )
  }

  fun resetOrientation() {
    update(
      CameraOptions().apply {
        bearing = 0.0
        pitch = 0.0
      },
      RESET_ANIMATION_MS,
    )
  }

  /**
   * Submits the map's extent. Skiko owns the texture this session borrows, so target replacement
   * carries only the graphics resource and the map resize is the sole extent authority here.
   */
  fun resize(extent: SurfaceExtent) {
    val size = MapSize(extent.width, extent.height, extent.scaleFactor)
    if (size != currentSize) {
      currentSize = size
      map.resize(size)
    }
  }

  /** Drains runtime events during the native-surface producer's render turn. */
  fun pollEvents() {
    if (drainEvents()) requestRender()
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
    private const val KEYBOARD_ANIMATION_MS = 160.0
    private const val RESET_ANIMATION_MS = 160.0
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
