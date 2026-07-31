package org.maplibre.nativeffi.examples.lwjglmap

import java.util.ArrayDeque
import kotlin.math.max
import kotlin.math.min
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
 * Runtime and map, owned for their whole lifetime by the runtime loop thread.
 *
 * The render target is not here: it belongs to the render loop thread, which owns the window and
 * the graphics context.
 */
internal class MapState
private constructor(private val runtime: RuntimeHandle, val map: MapHandle) : AutoCloseable {

  /** Acquires the wake source the render loop uses to release this loop's parked pump. */
  fun acquireWakeSource(): WakeSource = runtime.acquireWakeSource()

  /** Reused across drains, so applying a batch allocates nothing. */
  private var batch = ArrayDeque<CameraCommand>()

  /** One runtime loop iteration: apply queued commands, pump once, drain events. */
  fun step(commands: CommandQueue, renderRequest: RenderRequest) {
    batch = commands.drain(batch)
    for (command in batch) {
      apply(command)
    }
    // This thread has no display to pace it, so it takes its cadence from the runtime's own work
    // and parks in between. The render loop signals the wake source, so the bound is a backstop
    // rather than the cadence.
    runtime.pump(PARK_TIMEOUT_MS)
    if (drainEvents()) {
      renderRequest.set()
    }
  }

  /**
   * Applies one decoded camera command. Runs on the map's thread, which is why the
   * read-modify-write commands read the current camera here rather than on the render loop that
   * produced them.
   */
  private fun apply(command: CameraCommand) {
    when (command) {
      is CameraCommand.CancelTransitions -> map.cancelTransitions()
      is CameraCommand.SetGestureInProgress -> map.isGestureInProgress = command.inProgress
      is CameraCommand.MoveBy -> map.moveBy(command.dx, command.dy)
      is CameraCommand.MoveByAnimated ->
        map.moveByAnimated(command.dx, command.dy, animation(command.durationMs))

      is CameraCommand.ScaleBy -> map.scaleBy(command.scale, command.anchor)
      is CameraCommand.ScaleByAnimated ->
        map.scaleByAnimated(command.scale, command.anchor, animation(command.durationMs))

      is CameraCommand.PitchBy -> map.pitchBy(command.delta)
      is CameraCommand.AdjustBearing -> map.jumpTo(bearingCamera(command.delta))
      is CameraCommand.AdjustBearingAnimated ->
        map.easeTo(bearingCamera(command.delta), animation(command.durationMs))

      is CameraCommand.AdjustPitchAnimated ->
        map.easeTo(pitchCamera(command.delta), animation(command.durationMs))

      is CameraCommand.ResetOrientation ->
        map.easeTo(
          CameraOptions().apply {
            bearing = 0.0
            pitch = 0.0
          },
          animation(command.durationMs),
        )
    }
  }

  private fun bearingCamera(delta: Double): CameraOptions =
    CameraOptions().apply { bearing = (map.camera.bearing ?: 0.0) + delta }

  private fun pitchCamera(delta: Double): CameraOptions =
    CameraOptions().apply { pitch = max(0.0, min(60.0, (map.camera.pitch ?: 0.0) + delta)) }

  /** Drains runtime events, reporting whether the map wants another frame. */
  private fun drainEvents(): Boolean {
    var renderUpdateAvailable = false
    while (true) {
      val event = runtime.pollEvent() ?: return renderUpdateAvailable
      if (event.mapSource != map) {
        continue
      }
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

  override fun close() {
    try {
      map.close()
    } finally {
      runtime.close()
    }
  }

  companion object {
    private const val STYLE_URL = "https://tiles.openfreemap.org/styles/bright"

    /**
     * Backstop for the runtime loop's park. The render loop's wake source is what normally releases
     * it, so this only bounds a pump that nothing signals.
     */
    private const val PARK_TIMEOUT_MS = 100L

    fun create(viewport: Viewport): MapState {
      val runtime = RuntimeHandle.create(RuntimeOptions().apply { cachePath = ":memory:" })
      val mapOptions =
        MapOptions().apply {
          width = viewport.width()
          height = viewport.height()
          scaleFactor = viewport.scaleFactor()
          mapMode = MapMode.CONTINUOUS
        }
      val map =
        try {
          MapHandle.create(runtime, mapOptions)
        } catch (error: RuntimeException) {
          runtime.close()
          throw error
        }
      try {
        map.setStyleUrl(STYLE_URL)
        map.jumpTo(
          CameraOptions().apply {
            center = LatLng(37.7749, -122.4194)
            zoom = 13.0
            bearing = 12.0
            pitch = 30.0
          }
        )
        return MapState(runtime, map)
      } catch (error: RuntimeException) {
        map.close()
        runtime.close()
        throw error
      }
    }

    private fun animation(durationMs: Double): AnimationOptions =
      AnimationOptions().apply { this.durationMs = durationMs }
  }
}
