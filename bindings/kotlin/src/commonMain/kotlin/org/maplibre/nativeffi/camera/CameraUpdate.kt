package org.maplibre.nativeffi.camera

import org.maplibre.nativeffi.geo.ScreenPoint

/** Transition behavior for one atomic camera update. */
public enum class CameraUpdateMode(internal val nativeValue: Int) {
  JUMP(0),
  EASE(1),
  FLY(2),
}

/** Gesture boundary carried with one camera update. */
public enum class GesturePhase(internal val nativeValue: Int) {
  NONE(0),
  BEGIN(1),
  UPDATE(2),
  END(3),
  CANCEL(4),
}

/** One atomic camera command copied by the native runtime. */
public data class CameraUpdate(
  public val mode: CameraUpdateMode = CameraUpdateMode.JUMP,
  public val camera: CameraOptions = CameraOptions(),
  public val animation: AnimationOptions = AnimationOptions(),
  public val gesturePhase: GesturePhase = GesturePhase.NONE,
)

/** Relative camera operation kind. */
public enum class CameraDeltaKind(internal val nativeValue: Int) {
  MOVE(0),
  SCALE(1),
  BEARING(2),
  PITCH(3),
}

/** One relative camera operation. */
public data class CameraDelta(
  public val kind: CameraDeltaKind = CameraDeltaKind.MOVE,
  public val offset: ScreenPoint = ScreenPoint(0.0, 0.0),
  public val amount: Double = 0.0,
  public val anchor: ScreenPoint? = null,
  public val animation: AnimationOptions = AnimationOptions(),
)

/** Camera state and the map generation that published it. */
public data class CameraSnapshot(public val generation: Long, public val camera: CameraOptions)
