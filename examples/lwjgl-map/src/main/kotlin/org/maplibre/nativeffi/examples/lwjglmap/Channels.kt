package org.maplibre.nativeffi.examples.lwjglmap

import java.util.ArrayDeque
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicReference
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.map.MapHandle

/**
 * A camera change decoded on the render loop and applied on the map's thread.
 *
 * Commands carry deltas rather than absolute targets wherever the map's current camera is an input,
 * because reading the camera and writing the new one has to happen together on the thread that owns
 * the map.
 */
internal sealed interface CameraCommand {
  data object CancelTransitions : CameraCommand

  data class MoveBy(val dx: Double, val dy: Double) : CameraCommand

  data class MoveByAnimated(val dx: Double, val dy: Double, val durationMs: Double) : CameraCommand

  data class ScaleBy(val scale: Double, val anchor: ScreenPoint) : CameraCommand

  data class ScaleByAnimated(val scale: Double, val anchor: ScreenPoint, val durationMs: Double) :
    CameraCommand

  data class PitchBy(val delta: Double) : CameraCommand

  data class AdjustBearing(val delta: Double) : CameraCommand

  data class AdjustBearingAnimated(val delta: Double, val durationMs: Double) : CameraCommand

  data class AdjustPitchAnimated(val delta: Double, val durationMs: Double) : CameraCommand

  data class ResetOrientation(val durationMs: Double) : CameraCommand
}

/**
 * Ring of pending camera commands, filled by the render loop and drained by the runtime loop.
 *
 * Overflow drops the oldest command, because a dropped pan beats blocking the render loop on the
 * runtime loop.
 */
internal class CommandQueue {
  private val items = ArrayDeque<CameraCommand>(CAPACITY)

  fun push(command: CameraCommand) {
    synchronized(items) {
      if (items.size == CAPACITY) {
        items.removeFirst()
      }
      items.addLast(command)
    }
  }

  fun drain(): List<CameraCommand> =
    synchronized(items) {
      if (items.isEmpty()) {
        emptyList()
      } else {
        ArrayList(items).also { items.clear() }
      }
    }

  private companion object {
    const val CAPACITY = 256
  }
}

/**
 * One-bit signal that a frame is worth drawing.
 *
 * The render loop consumes before it renders and sets again when nothing was rendered, so a request
 * the runtime loop publishes during a render is not lost.
 */
internal class RenderRequest {
  private val requested = AtomicBoolean(true)

  fun set() {
    requested.set(true)
  }

  fun consume(): Boolean = requested.getAndSet(false)
}

/**
 * Publishes the map from the runtime loop to the render loop, and carries shutdown and failure the
 * other way.
 *
 * The render loop uses the published handle only to attach its own render session, which native
 * serves from any thread; every other map call stays on the runtime loop.
 */
internal class MapChannel {
  private val map = AtomicReference<MapHandle?>(null)
  private val shutdown = AtomicBoolean(false)
  private val failure = AtomicReference<Throwable?>(null)

  /** Runtime loop: announces the map it just created. */
  fun publishMap(handle: MapHandle) {
    map.set(handle)
  }

  /** Render loop: the map to attach against, once the runtime loop has one. */
  fun mapHandle(): MapHandle? = map.get()

  /**
   * Render loop: asks the runtime loop to stop. Called only after the render session is closed,
   * because the map cannot be destroyed before then.
   */
  fun requestShutdown() {
    shutdown.set(true)
  }

  fun shutdownRequested(): Boolean = shutdown.get()

  fun fail(error: Throwable) {
    failure.compareAndSet(null, error)
  }

  fun failure(): Throwable? = failure.get()
}
