package org.maplibre.nativeffi.examples.lwjglmap

import java.util.concurrent.atomic.AtomicBoolean
import org.maplibre.nativeffi.geo.ScreenPoint

/** A camera change decoded by GLFW and submitted directly to the map worker. */
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

/** A one-bit frame request shared with notification callbacks. */
internal class RenderRequest {
  private val requested = AtomicBoolean(true)

  fun set() {
    requested.set(true)
  }

  fun consume(): Boolean = requested.getAndSet(false)
}
