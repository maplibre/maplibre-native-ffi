package org.maplibre.nativeffi.style

/**
 * Mutable descriptor for the style's global paint-property transition options, distinct from camera
 * animation options and from the per-property transitions a style declares.
 *
 * Compares and hashes by field value; [copy] returns an independent instance. Keep an instance
 * unmodified while it is a key in a hash-based collection.
 */
public class StyleTransitionOptions {
  /**
   * Transition duration in milliseconds. Null falls back to the duration the style declares for
   * each transitioning property.
   */
  public var durationMs: Double? = null

  /**
   * Transition delay in milliseconds. Null falls back to the delay the style declares for each
   * transitioning property.
   */
  public var delayMs: Double? = null

  /**
   * Whether symbol placement changes cross-fade. Null leaves the cross-fade on; clearing it makes
   * placement changes apply to the next rendered frame. Reading the options always reports a value.
   */
  public var enablePlacementTransitions: Boolean? = null

  /** Returns an independent copy of this descriptor with [block] applied to the copy. */
  public fun copy(block: StyleTransitionOptions.() -> Unit = {}): StyleTransitionOptions =
    StyleTransitionOptions()
      .also {
        it.durationMs = durationMs
        it.delayMs = delayMs
        it.enablePlacementTransitions = enablePlacementTransitions
      }
      .apply(block)

  private val fields: List<Any?>
    get() = listOf(durationMs, delayMs, enablePlacementTransitions)

  override fun equals(other: Any?): Boolean =
    other is StyleTransitionOptions && fields == other.fields

  override fun hashCode(): Int = fields.hashCode()
}
