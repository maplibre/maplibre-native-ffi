package org.maplibre.nativeffi.camera

/**
 * Mutable animation descriptor used for animated camera commands.
 *
 * Compares and hashes by field value; [copy] returns an independent instance. Keep an instance
 * unmodified while it is a key in a hash-based collection.
 */
public class AnimationOptions {
  public var durationMs: Double? = null

  public var velocity: Double? = null

  public var minZoom: Double? = null

  public var easing: UnitBezier? = null

  /**
   * Caller-chosen identity for the transition these options start. Leaving it null leaves the
   * transition silent.
   *
   * When set, the transition emits one
   * [org.maplibre.nativeffi.runtime.RuntimeEventType.MAP_CAMERA_TRANSITION_FINISHED] event carrying
   * this value, however the transition ends: completed, superseded, cancelled, or applied instantly
   * as a zero-duration jump. The event reports only that the transition released the camera, not
   * the outcome. A rejected command starts no transition and emits no event.
   *
   * The value is opaque to MapLibre Native. Native `uint64_t` carried as a [Long] bit pattern.
   */
  public var transitionId: Long? = null

  /** Returns an independent copy of this descriptor with [block] applied to the copy. */
  public fun copy(block: AnimationOptions.() -> Unit = {}): AnimationOptions =
    AnimationOptions()
      .also {
        it.durationMs = durationMs
        it.velocity = velocity
        it.minZoom = minZoom
        it.easing = easing
        it.transitionId = transitionId
      }
      .apply(block)

  private val fields: List<Any?>
    get() = listOf(durationMs, velocity, minZoom, easing, transitionId)

  override fun equals(other: Any?): Boolean = other is AnimationOptions && fields == other.fields

  override fun hashCode(): Int = fields.hashCode()
}
