package org.maplibre.nativeffi.examples.androidmap

import java.util.concurrent.atomic.AtomicBoolean
import org.maplibre.nativeffi.geo.ScreenPoint

/** A camera change decoded on the UI thread and submitted directly to the map worker. */
internal sealed interface CameraCommand {
  data object CancelTransitions : CameraCommand

  data class SetGestureInProgress(val inProgress: Boolean) : CameraCommand

  data class MoveBy(val deltaX: Double, val deltaY: Double) : CameraCommand

  data class ScaleBy(val scale: Double, val anchor: ScreenPoint) : CameraCommand

  data class AdjustBearing(val degrees: Double, val anchor: ScreenPoint) : CameraCommand

  data class AdjustPitch(val degrees: Double) : CameraCommand

  /** Double tap: animate to one zoom level past the nearest whole level, about [anchor]. */
  data class ZoomToNextWholeLevel(val anchor: ScreenPoint) : CameraCommand
}

/**
 * One-bit signal that a frame is worth drawing. The render loop consumes before it renders and sets
 * again when nothing was rendered, so a request published during a render is not lost.
 */
internal class RenderRequest {
  private val value = AtomicBoolean(true)

  fun set() {
    value.set(true)
  }

  fun consume(): Boolean = value.getAndSet(false)
}
