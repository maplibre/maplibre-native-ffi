package org.maplibre.nativeffi.examples.composemap.map

import kotlinx.coroutines.Deferred
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.runBlocking
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.examples.composemap.surface.SurfaceExtent
import org.maplibre.nativeffi.generated.AnimationOptions
import org.maplibre.nativeffi.generated.CameraDelta
import org.maplibre.nativeffi.generated.CameraDeltaKind
import org.maplibre.nativeffi.generated.CameraOptions
import org.maplibre.nativeffi.generated.CameraUpdate
import org.maplibre.nativeffi.generated.CameraUpdateMode
import org.maplibre.nativeffi.generated.CommandDisposition
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
import org.maplibre.nativeffi.runtime.CommandCompletion

/**
 * The runtime and its map. Commands go straight to the runtime's own thread, and the runtime raises
 * [eventWake] when it has events to drain.
 */
internal class MapState(initialExtent: SurfaceExtent, eventWake: Wake, styleJson: String? = null) :
  AutoCloseable {
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
      // A rejection here fails construction; a terminal failure is only printed.
      val style =
        if (styleJson != null) map.setStyleJson(styleJson.encodeToByteArray())
        else map.setStyleUrl(STYLE_URL)
      style.reportFailure("style load")
      map.updateCamera(CameraUpdate(camera = initialCamera)).reportFailure("initial camera")
    } catch (error: Throwable) {
      runBlocking {
        if (::ownedMap.isInitialized) ownedMap.release().await()
        runtime.release().await()
      }
      throw error
    }
  }

  fun cancelTransitions() {
    submit("camera transition cancel") { map.cancelTransitions() }
  }

  fun setGestureInProgress(inProgress: Boolean) {
    submit("gesture update") {
      map.updateCamera(
        CameraUpdate(gesturePhase = if (inProgress) GesturePhase.BEGIN else GesturePhase.END)
      )
    }
  }

  fun moveBy(deltaX: Double, deltaY: Double) {
    delta(CameraDelta(offset = ScreenPoint(deltaX, deltaY)))
  }

  fun moveByAnimated(deltaX: Double, deltaY: Double) {
    delta(
      CameraDelta(
        offset = ScreenPoint(deltaX, deltaY),
        animation = animation(KEYBOARD_ANIMATION_MS),
      )
    )
  }

  fun scaleBy(scale: Double, anchor: ScreenPoint) {
    delta(CameraDelta(kind = CameraDeltaKind.SCALE, amount = scale, anchor = anchor))
  }

  fun scaleByAnimated(scale: Double, anchor: ScreenPoint) {
    delta(
      CameraDelta(
        kind = CameraDeltaKind.SCALE,
        amount = scale,
        anchor = anchor,
        animation = animation(KEYBOARD_ANIMATION_MS),
      )
    )
  }

  fun adjustBearingAndPitch(bearingDegrees: Double, pitchDegrees: Double) {
    delta(CameraDelta(kind = CameraDeltaKind.BEARING, amount = bearingDegrees))
    delta(CameraDelta(kind = CameraDeltaKind.PITCH, amount = pitchDegrees))
  }

  fun adjustBearingAnimated(bearingDegrees: Double) {
    delta(
      CameraDelta(
        kind = CameraDeltaKind.BEARING,
        amount = bearingDegrees,
        animation = animation(KEYBOARD_ANIMATION_MS),
      )
    )
  }

  fun adjustPitchAnimated(pitchDegrees: Double) {
    delta(
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
      submit("map resize") { map.resize(size) }
    }
  }

  private fun update(camera: CameraOptions, durationMs: Double? = null) {
    submit("camera update") {
      map.updateCamera(
        CameraUpdate(
          mode = if (durationMs == null) CameraUpdateMode.JUMP else CameraUpdateMode.EASE,
          camera = camera,
          animation = AnimationOptions(durationMs = durationMs),
        )
      )
    }
  }

  private fun delta(delta: CameraDelta) {
    submit("camera delta") { map.applyCameraDelta(delta) }
  }

  /**
   * Submits a command without waiting on it. A rejection or a terminal failure is printed, so that
   * one bad input does not escape an input handler or the render callback.
   */
  private inline fun submit(operation: String, command: () -> Deferred<CommandCompletion>) {
    try {
      command().reportFailure(operation)
    } catch (error: MaplibreException) {
      System.err.println("$operation rejected: $error")
    }
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

/**
 * Prints the command's failure once it completes: an error, or a FAILED terminal disposition. A
 * superseded or cancelled command ended without failing, so it prints nothing.
 */
@OptIn(ExperimentalCoroutinesApi::class)
private fun Deferred<CommandCompletion>.reportFailure(operation: String) {
  invokeOnCompletion { error ->
    if (error != null) {
      System.err.println("$operation failed: $error")
      return@invokeOnCompletion
    }
    val completion = getCompleted()
    if (completion.disposition == CommandDisposition.FAILED) {
      System.err.println("$operation failed: ${completion.status}: ${completion.diagnostic}")
    }
  }
}
