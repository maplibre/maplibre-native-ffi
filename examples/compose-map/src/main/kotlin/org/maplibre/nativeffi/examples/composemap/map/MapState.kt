package org.maplibre.nativeffi.examples.composemap.map

import kotlinx.coroutines.runBlocking
import org.maplibre.nativeffi.examples.composemap.surface.SurfaceExtent
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
import org.maplibre.nativeffi.generated.ScreenPoint
import org.maplibre.nativeffi.generated.Wake

/**
 * The runtime and its map. Commands go straight to the runtime's own thread, and the runtime raises
 * [eventWake] when it has events to drain.
 */
internal class MapState(initialExtent: SurfaceExtent, eventWake: Wake, styleJson: String? = null) :
  AutoCloseable {
  private var closed = false
  private var currentSize = initialExtent.toLogicalExtent()
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
          .mapCreate(
            GeneratedApi.mapOptionsDefault()
              .copy(
                initialExtent = currentSize,
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

  fun moveByAnimated(deltaX: Double, deltaY: Double) {
    map.applyCameraDelta(
      CameraDelta(
        offset = ScreenPoint(deltaX, deltaY),
        animation = animation(KEYBOARD_ANIMATION_MS),
      )
    )
  }

  fun scaleBy(scale: Double, anchor: ScreenPoint) {
    map.applyCameraDelta(CameraDelta(scale = scale, anchor = anchor))
  }

  fun scaleByAnimated(scale: Double, anchor: ScreenPoint) {
    map.applyCameraDelta(
      CameraDelta(scale = scale, anchor = anchor, animation = animation(KEYBOARD_ANIMATION_MS))
    )
  }

  fun adjustBearingAndPitch(bearingDegrees: Double, pitchDegrees: Double) {
    map.applyCameraDelta(CameraDelta(bearing = bearingDegrees, pitch = pitchDegrees))
  }

  fun adjustBearingAnimated(bearingDegrees: Double) {
    map.applyCameraDelta(
      CameraDelta(bearing = bearingDegrees, animation = animation(KEYBOARD_ANIMATION_MS))
    )
  }

  fun adjustPitchAnimated(pitchDegrees: Double) {
    map.applyCameraDelta(
      CameraDelta(pitch = pitchDegrees, animation = animation(KEYBOARD_ANIMATION_MS))
    )
  }

  fun resetOrientation() {
    update(CameraOptions(bearing = 0.0, pitch = 0.0), RESET_ANIMATION_MS)
  }

  /**
   * Submits the map's extent. Skiko owns the texture this session borrows, so target replacement
   * carries only the graphics resource and the map resize is the sole extent authority here.
   */
  fun resize(extent: SurfaceExtent) {
    val size = extent.toLogicalExtent()
    if (size != currentSize) {
      currentSize = size
      map.resize(size)
    }
  }

  private fun update(camera: CameraOptions, durationMs: Double? = null) {
    map.updateCamera(
      CameraUpdate(
        mode = if (durationMs == null) CameraUpdateMode.JUMP else CameraUpdateMode.EASE,
        camera = camera,
        animation = AnimationOptions(durationMs = durationMs),
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
    private const val KEYBOARD_ANIMATION_MS = 160.0
    private const val RESET_ANIMATION_MS = 160.0
  }
}
