package org.maplibre.nativeffi.examples.composemap.map

import kotlinx.coroutines.Deferred
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.runBlocking
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.examples.composemap.surface.SurfaceExtent
import org.maplibre.nativeffi.generated.AnimationOptions
import org.maplibre.nativeffi.generated.CameraDelta
import org.maplibre.nativeffi.generated.CameraOptions
import org.maplibre.nativeffi.generated.CameraUpdate
import org.maplibre.nativeffi.generated.CameraUpdateMode
import org.maplibre.nativeffi.generated.CommandDisposition
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.GesturePhase
import org.maplibre.nativeffi.generated.LatLng
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.MapMode
import org.maplibre.nativeffi.generated.RuntimeEventMask
import org.maplibre.nativeffi.generated.ScreenPoint
import org.maplibre.nativeffi.runtime.CommandCompletion

/** The runtime and its map. Commands run on the runtime's scheduler thread. */
internal class MapState(initialExtent: SurfaceExtent, styleJson: String? = null) : AutoCloseable {
  private var closed = false
  private val initialCamera =
    CameraOptions(center = LatLng(37.7749, -122.4194), zoom = 13.0, bearing = 12.0, pitch = 30.0)

  // Creation waits for the runtime's thread to start.
  private val runtime = runBlocking {
    GeneratedApi.runtimeCreate(
      GeneratedApi.runtimeOptionsDefault().copy(cachePath = ":memory:"),
    )
  }
  private lateinit var ownedMap: MapHandle
  val map: MapHandle
    get() = ownedMap

  init {
    try {
      ownedMap = runBlocking {
        runtime
          .createMap(
            GeneratedApi.mapOptionsDefault()
              .copy(
                initialExtent = initialExtent.toLogicalExtent(),
                mapMode = MapMode.CONTINUOUS,
                eventMask = RuntimeEventMask.NONE,
              ),
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
        CameraUpdate(gesturePhase = if (inProgress) GesturePhase.BEGIN else GesturePhase.END),
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
      ),
    )
  }

  fun scaleBy(scale: Double, anchor: ScreenPoint) {
    delta(CameraDelta(scale = scale, anchor = anchor))
  }

  fun scaleByAnimated(scale: Double, anchor: ScreenPoint) {
    delta(CameraDelta(scale = scale, anchor = anchor, animation = animation(KEYBOARD_ANIMATION_MS)))
  }

  fun adjustBearingAndPitch(bearingDegrees: Double, pitchDegrees: Double) {
    delta(CameraDelta(bearing = bearingDegrees, pitch = pitchDegrees))
  }

  fun adjustBearingAnimated(bearingDegrees: Double) {
    delta(CameraDelta(bearing = bearingDegrees, animation = animation(KEYBOARD_ANIMATION_MS)))
  }

  fun adjustPitchAnimated(pitchDegrees: Double) {
    delta(CameraDelta(pitch = pitchDegrees, animation = animation(KEYBOARD_ANIMATION_MS)))
  }

  fun resetOrientation() {
    update(CameraOptions(bearing = 0.0, pitch = 0.0), RESET_ANIMATION_MS)
  }

  private fun update(camera: CameraOptions, durationMs: Double? = null) {
    submit("camera update") {
      map.updateCamera(
        CameraUpdate(
          mode = if (durationMs == null) CameraUpdateMode.JUMP else CameraUpdateMode.EASE,
          camera = camera,
          animation = AnimationOptions(durationMs = durationMs),
        ),
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
