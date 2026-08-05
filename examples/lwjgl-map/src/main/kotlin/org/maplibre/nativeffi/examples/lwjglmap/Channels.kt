package org.maplibre.nativeffi.examples.lwjglmap

import java.util.ArrayDeque
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicReference
import kotlin.time.Duration.Companion.seconds
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.runtime.WakeSource

/**
 * A camera change decoded on the render loop and applied on the map's thread.
 *
 * Commands carry deltas rather than absolute targets, because reading the camera and writing the
 * new one has to happen together on the thread that owns the map.
 */
internal sealed interface CameraCommand {
  data object CancelTransitions : CameraCommand

  data class SetGestureInProgress(val inProgress: Boolean) : CameraCommand

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
 * Pending camera commands, filled by the render loop and drained by the runtime loop.
 *
 * The queue grows rather than dropping: its commands are deltas and a gesture bracket, and
 * discarding either corrupts the gesture.
 */
internal class CommandQueue {
  private val lock = Any()
  private var pending = ArrayDeque<CameraCommand>()

  /**
   * Released by [push] so a queued command reaches the runtime loop without waiting out its parking
   * bound. Set once the runtime loop has published its wake source.
   */
  @Volatile var onEnqueue: (() -> Unit)? = null

  fun push(command: CameraCommand) {
    synchronized(lock) { pending.addLast(command) }
    onEnqueue?.invoke()
  }

  /** Runtime loop: hands the pending deque over and takes [out] in exchange. */
  fun drain(out: ArrayDeque<CameraCommand>): ArrayDeque<CameraCommand> {
    out.clear()
    return synchronized(lock) {
      val drained = pending
      pending = out
      drained
    }
  }
}

/**
 * One-bit signal that a frame is worth drawing. The render loop consumes before it renders and sets
 * again when nothing was rendered, so a request published during a render is not lost.
 */
internal class RenderRequest {
  private val requested = AtomicBoolean(true)

  fun set() {
    requested.set(true)
  }

  fun consume(): Boolean = requested.getAndSet(false)
}

/**
 * Publishes the map and the runtime's wake source from the runtime loop to the render loop, and
 * carries shutdown and failure the other way.
 *
 * The render loop uses the published handle only to attach its own render session, which native
 * serves from any thread; every other map call stays on the runtime loop.
 */
internal class MapChannel {
  private val map = AtomicReference<MapHandle?>(null)
  private val wake = AtomicReference<WakeSource?>(null)
  private val shutdown = AtomicBoolean(false)
  private val failure = AtomicReference<Throwable?>(null)

  /** Runtime loop: announces the map it just created and its wake source. */
  fun publish(handle: MapHandle, source: WakeSource) {
    wake.set(source)
    map.set(handle)
  }

  /** Render loop: releases the runtime loop's parked pump. */
  fun wakeRuntimeLoop() {
    wake.get()?.signal()
  }

  /** Render loop: the map to attach against, once the runtime loop has one. */
  fun mapHandle(): MapHandle? = map.get()

  /**
   * Render loop: asks the runtime loop to stop. Called only after the render session is closed,
   * because the map cannot be destroyed before then.
   */
  fun requestShutdown() {
    shutdown.set(true)
    wakeRuntimeLoop()
  }

  fun shutdownRequested(): Boolean = shutdown.get()

  /**
   * Runtime loop: blocks until [requestShutdown], or until the bound expires, so a render loop that
   * stopped without signalling cannot wedge teardown.
   */
  fun awaitShutdown() {
    val deadline = System.nanoTime() + SHUTDOWN_WAIT.inWholeNanoseconds
    while (!shutdown.get() && System.nanoTime() < deadline) {
      Thread.sleep(SHUTDOWN_POLL_MS)
    }
  }

  fun fail(error: Throwable) {
    failure.compareAndSet(null, error)
  }

  fun failure(): Throwable? = failure.get()

  private companion object {
    val SHUTDOWN_WAIT = 5.seconds
    const val SHUTDOWN_POLL_MS = 2L
  }
}
