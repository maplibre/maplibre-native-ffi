package org.maplibre.nativeffi.examples.composemap.map

import java.util.concurrent.atomic.AtomicBoolean
import kotlinx.coroutines.runBlocking
import org.maplibre.nativeffi.examples.composemap.surface.SurfaceExtent
import org.maplibre.nativeffi.generated.AnimationOptions
import org.maplibre.nativeffi.generated.CameraDelta
import org.maplibre.nativeffi.generated.CameraDeltaKind
import org.maplibre.nativeffi.generated.CameraOptions
import org.maplibre.nativeffi.generated.CameraUpdate
import org.maplibre.nativeffi.generated.CameraUpdateMode
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
internal class MapState(
  initialExtent: SurfaceExtent,
  private val requestRender: () -> Unit,
  styleJson: String? = null,
) : AutoCloseable {
  private var closed = false
  private var currentSize =
    LogicalExtent(
      initialExtent.width.toUInt(),
      initialExtent.height.toUInt(),
      initialExtent.scaleFactor,
    )
  private val initialCamera =
    CameraOptions(center = LatLng(37.7749, -122.4194), zoom = 13.0, bearing = 12.0, pitch = 30.0)

  private val runtime =
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault().copy(cachePath = ":memory:"))
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
                initialExtent =
                  LogicalExtent(
                    initialExtent.width.toUInt(),
                    initialExtent.height.toUInt(),
                    initialExtent.scaleFactor,
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
    update(CameraOptions(bearing = 0.0, pitch = 0.0), RESET_ANIMATION_MS)
  }

  /**
   * Submits the map's extent. Skiko owns the texture this session borrows, so target replacement
   * carries only the graphics resource and the map resize is the sole extent authority here.
   */
  fun resize(extent: SurfaceExtent) {
    val size = LogicalExtent(extent.width.toUInt(), extent.height.toUInt(), extent.scaleFactor)
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
        animation = AnimationOptions(durationMs = durationMs),
      )
    )
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
