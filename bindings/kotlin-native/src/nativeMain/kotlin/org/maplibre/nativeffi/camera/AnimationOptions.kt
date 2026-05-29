package org.maplibre.nativeffi.camera

/** Mutable animation descriptor used for animated camera commands. */
public class AnimationOptions {
  public var durationMs: Double? = null
    private set

  public var velocity: Double? = null
    private set

  public var minZoom: Double? = null
    private set

  public var easing: UnitBezier? = null
    private set

  public fun hasDurationMs(): Boolean = durationMs != null

  public fun durationMs(durationMs: Double): AnimationOptions = apply {
    this.durationMs = durationMs
  }

  public fun clearDurationMs(): AnimationOptions = apply { durationMs = null }

  public fun hasVelocity(): Boolean = velocity != null

  public fun velocity(velocity: Double): AnimationOptions = apply { this.velocity = velocity }

  public fun clearVelocity(): AnimationOptions = apply { velocity = null }

  public fun hasMinZoom(): Boolean = minZoom != null

  public fun minZoom(minZoom: Double): AnimationOptions = apply { this.minZoom = minZoom }

  public fun clearMinZoom(): AnimationOptions = apply { minZoom = null }

  public fun hasEasing(): Boolean = easing != null

  public fun easing(easing: UnitBezier): AnimationOptions = apply { this.easing = easing }

  public fun clearEasing(): AnimationOptions = apply { easing = null }
}
