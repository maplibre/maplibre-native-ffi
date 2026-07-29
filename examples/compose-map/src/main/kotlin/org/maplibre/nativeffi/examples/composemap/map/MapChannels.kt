package org.maplibre.nativeffi.examples.composemap.map

import java.util.concurrent.ConcurrentLinkedQueue
import java.util.concurrent.atomic.AtomicBoolean
import org.maplibre.nativeffi.geo.ScreenPoint

/**
 * A camera change decoded on the render loop and applied on the map's owner thread.
 *
 * Commands carry deltas rather than absolute targets wherever the map's current camera is an input,
 * because reading the camera and writing the new one has to happen together on the thread that owns
 * the map.
 */
internal sealed interface CameraCommand {
  data object CancelTransitions : CameraCommand

  data class MoveBy(val deltaX: Double, val deltaY: Double) : CameraCommand

  data class MoveByAnimated(val deltaX: Double, val deltaY: Double) : CameraCommand

  data class ScaleBy(val scale: Double, val anchor: ScreenPoint) : CameraCommand

  data class ScaleByAnimated(val scale: Double, val anchor: ScreenPoint) : CameraCommand

  data class AdjustBearingAndPitch(val bearingDegrees: Double, val pitchDegrees: Double) :
    CameraCommand

  data class AdjustBearingAnimated(val bearingDegrees: Double) : CameraCommand

  data class AdjustPitchAnimated(val pitchDegrees: Double) : CameraCommand

  data object ResetOrientation : CameraCommand
}

/**
 * Queue of pending camera commands, written by input handlers and read by the runtime loop.
 *
 * [onEnqueue] releases the runtime loop's parked pump, so input reaches it without waiting out the
 * parking bound. The loop parks inside the native pump rather than on a host primitive, so the same
 * park also wakes for the runtime's own work.
 */
internal class CameraCommandQueue {
  private val queue = ConcurrentLinkedQueue<CameraCommand>()

  /** Set once the runtime loop has acquired its wake source. */
  @Volatile var onEnqueue: (() -> Unit)? = null

  fun enqueue(command: CameraCommand) {
    queue.add(command)
    wake()
  }

  /** Moves every queued command into [out] so the runtime loop can apply them in order. */
  fun drainInto(out: MutableList<CameraCommand>) {
    while (true) {
      out.add(queue.poll() ?: return)
    }
  }

  fun wake() {
    onEnqueue?.invoke()
  }
}

/**
 * One-bit signal that a frame is worth drawing.
 *
 * The render loop consumes before it renders and sets again when nothing was rendered, so a request
 * the runtime loop publishes during a render is not lost.
 */
internal class RenderRequest {
  private val value = AtomicBoolean(true)

  fun set() {
    value.set(true)
  }

  fun consume(): Boolean = value.getAndSet(false)
}
