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

  /** Returns an independent copy of this descriptor with [block] applied to the copy. */
  public fun copy(block: AnimationOptions.() -> Unit = {}): AnimationOptions =
    AnimationOptions()
      .also {
        it.durationMs = durationMs
        it.velocity = velocity
        it.minZoom = minZoom
        it.easing = easing
      }
      .apply(block)

  private val fields: List<Any?>
    get() = listOf(durationMs, velocity, minZoom, easing)

  override fun equals(other: Any?): Boolean = other is AnimationOptions && fields == other.fields

  override fun hashCode(): Int = fields.hashCode()
}
