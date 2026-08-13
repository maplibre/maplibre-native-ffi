package org.maplibre.nativeffi.examples.composemap.map

import java.util.concurrent.atomic.AtomicBoolean
import org.maplibre.nativeffi.geo.ScreenPoint

/** A camera change submitted directly from a Compose input handler to the map worker. */
internal sealed interface CameraCommand {
  data object CancelTransitions : CameraCommand

  data class SetGestureInProgress(val inProgress: Boolean) : CameraCommand

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

/** A one-bit frame request shared with notification callbacks. */
internal class RenderRequest {
  private val value = AtomicBoolean(true)

  fun set() {
    value.set(true)
  }

  fun consume(): Boolean = value.getAndSet(false)
}
