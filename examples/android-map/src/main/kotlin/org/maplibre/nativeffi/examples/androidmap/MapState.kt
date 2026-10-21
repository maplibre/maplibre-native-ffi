package org.maplibre.nativeffi.examples.androidmap

import android.util.Log
import kotlin.math.pow
import kotlin.math.round
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.runBlocking
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.generated.AnimationOptions
import org.maplibre.nativeffi.generated.CameraDelta
import org.maplibre.nativeffi.generated.CameraOptions
import org.maplibre.nativeffi.generated.CameraUpdate
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
internal class MapState(initialViewport: Viewport, styleJson: String? = null) : AutoCloseable {
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
                initialExtent = initialViewport.extent,
                mapMode = MapMode.CONTINUOUS,
                eventMask = RuntimeEventMask.NONE,
              ),
          )
          .await()
      }
      // A rejection here fails construction; a terminal failure is only logged.
      val style =
        if (styleJson != null) map.setStyleJson(styleJson.encodeToByteArray())
        else map.setStyleUrl(STYLE_URL)
      style.logFailure("style load")
      map.updateCamera(CameraUpdate(camera = initialCamera)).logFailure("initial camera")
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

  /**
   * Pans by the centroid's movement, then scales and turns about the new centroid, in one delta, so
   * no frame shows the pan without the zoom and turn.
   */
  fun pinchBy(offset: ScreenPoint, scale: Double, bearingDegrees: Double, centroid: ScreenPoint) {
    delta(CameraDelta(offset = offset, scale = scale, bearing = bearingDegrees, anchor = centroid))
  }

  fun adjustPitch(degrees: Double) {
    delta(CameraDelta(pitch = degrees))
  }

  /** Eases to the next whole zoom level, as `round(zoom) + 1`, about [anchor]. */
  fun zoomToNextWholeLevel(anchor: ScreenPoint) {
    val zoom = map.getSnapshot().view.camera.zoom ?: 0.0
    delta(
      CameraDelta(
        scale = 2.0.pow(round(zoom) + 1.0 - zoom),
        anchor = anchor,
        animation = animation(DOUBLE_TAP_DURATION_MS),
      ),
    )
  }

  private fun delta(delta: CameraDelta) {
    submit("camera delta") { map.applyCameraDelta(delta) }
  }

  /**
   * Submits a command without waiting on it. A rejection or a terminal failure is logged, so that
   * one bad input does not escape a touch or surface callback.
   */
  private inline fun submit(operation: String, command: () -> Deferred<CommandCompletion>) {
    try {
      command().logFailure(operation)
    } catch (error: MaplibreException) {
      Log.w(TAG, "$operation rejected", error)
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
    private const val DOUBLE_TAP_DURATION_MS = 160.0
  }
}

private const val TAG = "MapLibreAndroidMap"

/**
 * Logs the command's failure once it completes: an error, or a FAILED terminal disposition. A
 * superseded or cancelled command ended without failing, so it logs nothing.
 */
private fun Deferred<CommandCompletion>.logFailure(operation: String) {
  invokeOnCompletion { error ->
    (error ?: terminalFailure(operation))?.let { Log.w(TAG, "$operation failed", it) }
  }
}

/** The failure that a completed command reports as its terminal disposition, or null. */
@OptIn(ExperimentalCoroutinesApi::class)
internal fun Deferred<CommandCompletion>.terminalFailure(operation: String): Throwable? =
  getCompleted()
    .takeIf { it.disposition == CommandDisposition.FAILED }
    ?.let { IllegalStateException("$operation failed: ${it.status}: ${it.diagnostic}") }
