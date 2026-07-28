package org.maplibre.nativeffi.examples.androidmap

import android.os.Handler
import android.os.HandlerThread
import android.util.Log
import java.util.concurrent.ConcurrentLinkedQueue
import kotlin.math.pow
import kotlin.math.round
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.runtime.RuntimeEventPayload
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions
import org.maplibre.nativeffi.runtime.WakeSource

/**
 * Owns the runtime and the map for their whole lifetime, off the UI thread.
 *
 * Native owner-thread checks are keyed on the OS thread, so this is a [HandlerThread] rather than a
 * coroutine dispatcher or a pooled executor. The loop never touches the render session: the render
 * loop attaches its own session against the map published here, and closes it before [close] lets
 * this loop destroy the map.
 *
 * The loop keeps pumping while the view is off screen or the app is backgrounded, so style and tile
 * loading continue.
 */
internal class MapRuntimeLoop(private val initialViewport: Viewport) : AutoCloseable {
  private val commands = ConcurrentLinkedQueue<CameraCommand>()
  private val thread = HandlerThread("maplibre-runtime").apply { start() }
  private val handler = Handler(thread.looper)
  private var runtime: RuntimeHandle? = null
  private var owned: MapHandle? = null

  /**
   * Releases the parked pump on the handler thread. Every path that posts work to that thread
   * signals this too, because a parked pump would otherwise hold the message until its bound.
   */
  @Volatile private var wake: WakeSource? = null

  /** The map to attach against, once the loop has created one. */
  @Volatile
  var map: MapHandle? = null
    private set

  /**
   * Set when setup failed, so the view stops scheduling frames instead of reposting every vsync
   * against a loop that will never publish a map.
   */
  @Volatile
  var setupFailure: Throwable? = null
    private set

  /** Set by the runtime loop when the map wants another frame, consumed by the render loop. */
  val renderRequest: RenderRequest = RenderRequest()

  private val pump =
    object : Runnable {
      override fun run() {
        handler.removeCallbacks(this)
        val currentRuntime = runtime
        val currentMap = owned
        if (currentRuntime != null && currentMap != null) {
          try {
            while (true) {
              apply(currentMap, commands.poll() ?: break)
            }
            // This thread has no display to pace it, so it takes its cadence from the runtime's
            // own work and parks in between. The render loop signals the wake source on enqueue,
            // and close() signals it too, so the bound is a backstop rather than the cadence.
            currentRuntime.pump(PARK_TIMEOUT_MS)
            if (drainEvents(currentRuntime, currentMap)) {
              renderRequest.set()
            }
          } catch (error: RuntimeException) {
            // Reposting after a failing pump spins the handler thread forever with nothing to
            // show for it. Record it so the view stops scheduling frames, and stop the loop.
            Log.e(TAG, "runtime loop iteration failed", error)
            setupFailure = error
            closeHandles()
            return
          }
        }
        // The pump above provides the pacing, so requeue immediately rather than adding a delay
        // on top of the park.
        handler.post(this)
      }
    }

  init {
    handler.post { create() }
  }

  fun enqueue(command: CameraCommand) {
    commands.add(command)
    handler.post(pump)
    wake?.signal()
  }

  /** Asks the map to redraw, for instance after the render loop attached a fresh session. */
  fun requestRepaint() {
    handler.post { owned?.requestRepaint() }
    wake?.signal()
  }

  override fun close() {
    handler.removeCallbacks(pump)
    handler.post { closeHandles() }
    // Release the parked pump so the close above runs now rather than after its bound.
    wake?.signal()
    thread.quitSafely()
    thread.join()
  }

  private fun create() {
    try {
      val createdRuntime = RuntimeHandle.create(RuntimeOptions().apply { cachePath = ":memory:" })
      runtime = createdRuntime
      wake = createdRuntime.acquireWakeSource()
      val createdMap =
        MapHandle.create(
          createdRuntime,
          MapOptions().apply {
            width = initialViewport.logicalWidth
            height = initialViewport.logicalHeight
            scaleFactor = initialViewport.scaleFactor
            mapMode = MapMode.CONTINUOUS
          },
        )
      owned = createdMap
      createdMap.setStyleUrl(STYLE_URL)
      createdMap.jumpTo(
        CameraOptions().apply {
          center = LatLng(37.7749, -122.4194)
          zoom = 13.0
          bearing = 12.0
          pitch = 30.0
        }
      )
      map = createdMap
      handler.post(pump)
    } catch (error: RuntimeException) {
      Log.e(TAG, "runtime loop setup failed", error)
      setupFailure = error
      closeHandles()
    }
  }

  private fun closeHandles() {
    map = null
    val closingMap = owned
    val closingRuntime = runtime
    val closingWake = wake
    owned = null
    runtime = null
    wake = null
    try {
      closingMap?.close()
    } finally {
      try {
        closingRuntime?.close()
      } finally {
        // A wake source outlives its runtime, so this is safe in either order.
        closingWake?.close()
      }
    }
  }

  /**
   * Applies one decoded camera command on the map's owner thread, which is why the
   * read-modify-write commands read the current camera here rather than on the render loop that
   * produced them.
   */
  private fun apply(map: MapHandle, command: CameraCommand) {
    when (command) {
      CameraCommand.CancelTransitions -> map.cancelTransitions()
      is CameraCommand.MoveBy -> map.moveBy(command.deltaX, command.deltaY)
      is CameraCommand.ScaleBy -> map.scaleBy(command.scale, command.anchor)
      is CameraCommand.AdjustBearing -> {
        val camera = map.camera
        map.jumpTo(
          CameraOptions().apply {
            bearing = (camera.bearing ?: 0.0) + command.degrees
            anchor = command.anchor
          }
        )
      }
      is CameraCommand.AdjustPitch -> {
        val camera = map.camera
        map.jumpTo(
          CameraOptions().apply {
            pitch = ((camera.pitch ?: 0.0) + command.degrees).coerceIn(MIN_PITCH, MAX_PITCH)
          }
        )
      }
      is CameraCommand.ZoomToNextWholeLevel -> {
        val zoom = map.camera.zoom ?: 0.0
        val targetZoom = round(zoom) + 1.0
        map.scaleByAnimated(2.0.pow(targetZoom - zoom), command.anchor, DOUBLE_TAP_ANIMATION)
      }
    }
  }

  /** Drains runtime events, reporting whether the map wants another frame. */
  private fun drainEvents(runtime: RuntimeHandle, map: MapHandle): Boolean {
    var renderUpdateAvailable = false
    while (true) {
      val event = runtime.pollEvent() ?: return renderUpdateAvailable
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
  }

  private companion object {
    private const val TAG = "MapLibreRuntimeLoop"
    private const val STYLE_URL = "https://tiles.openfreemap.org/styles/bright"
    /**
     * Backstop for the runtime loop's park. The wake source is what normally releases it, so this
     * only bounds a pump that nothing signals.
     */
    private const val PARK_TIMEOUT_MS = 100L
    private const val MIN_PITCH = 0.0
    private const val MAX_PITCH = 60.0
    private val DOUBLE_TAP_ANIMATION = AnimationOptions().apply { durationMs = 160.0 }
  }
}
