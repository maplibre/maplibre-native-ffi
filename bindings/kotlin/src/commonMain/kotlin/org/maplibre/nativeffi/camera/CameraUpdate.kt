package org.maplibre.nativeffi.camera

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
  public val gestureId: Long = 0,
  public val animationId: Long = 0,
)

/** Camera state and the map generation that published it. */
public data class CameraSnapshot(public val generation: Long, public val camera: CameraOptions)
