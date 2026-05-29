package org.maplibre.nativeffi.camera

/** Mutable animation descriptor used for animated camera commands. */
public class AnimationOptions {
  public var durationMs: Double? = null

  public var velocity: Double? = null

  public var minZoom: Double? = null

  public var easing: UnitBezier? = null
}
