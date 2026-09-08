package org.maplibre.nativeffi.camera

import org.maplibre.nativeffi.geo.ScreenPoint

/** Transition behavior for one atomic camera update. */
public enum class CameraUpdateMode(internal val nativeValue: Int) {
  JUMP(0),
  EASE(1),
  FLY(2),
}

/**
 * Gesture boundary carried atomically with one camera update.
 *
 * The phase is applied around the camera write and is reported by
 * [org.maplibre.nativeffi.map.MapSnapshot.gestureInProgress].
 */
public enum class GesturePhase(internal val nativeValue: Int) {
  /** The update carries no gesture boundary and leaves the flag as it is. */
  NONE(0),
  /**
   * Marks a gesture as in progress before the camera write. It does not cancel running transitions;
   * use [org.maplibre.nativeffi.map.MapHandle.cancelTransitions] for that.
   */
  BEGIN(1),
  /** Keeps the gesture marked as in progress before the camera write. */
  UPDATE(2),
  /** Clears the gesture flag after the camera write. */
  END(3),
  /** Cancels transitions running after the camera write, then clears the gesture flag. */
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
  /** Adds the amount to the current pitch, the opposite sign of MapLibre Native's `pitchBy()`. */
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
