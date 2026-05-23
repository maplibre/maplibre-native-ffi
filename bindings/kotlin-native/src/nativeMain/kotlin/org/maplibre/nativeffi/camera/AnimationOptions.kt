package org.maplibre.nativeffi.camera

/** Mutable animation descriptor used for animated camera commands. */
public class AnimationOptions {
  public var durationMillis: Double? = null
    private set

  public var velocity: Double? = null
    private set

  public var minZoom: Double? = null
    private set

  public var easing: UnitBezier? = null
    private set

  public fun hasDurationMillis(): Boolean = durationMillis != null

  public fun durationMillis(durationMillis: Double): AnimationOptions = apply {
    this.durationMillis = durationMillis
  }

  public fun clearDurationMillis(): AnimationOptions = apply { durationMillis = null }

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
