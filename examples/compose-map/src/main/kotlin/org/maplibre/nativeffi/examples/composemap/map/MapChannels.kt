package org.maplibre.nativeffi.examples.composemap.map

import java.util.concurrent.ConcurrentLinkedQueue
import java.util.concurrent.Semaphore
import java.util.concurrent.TimeUnit
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
 * [await] lets the runtime loop pace itself while still waking as soon as input arrives.
 */
internal class CameraCommandQueue {
  private val queue = ConcurrentLinkedQueue<CameraCommand>()
  private val wakeup = Semaphore(0)

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

  /** Waits up to [timeoutMs] for new work. */
  fun await(timeoutMs: Long) {
    wakeup.tryAcquire(timeoutMs, TimeUnit.MILLISECONDS)
    wakeup.drainPermits()
  }

  fun wake() {
    wakeup.release()
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
