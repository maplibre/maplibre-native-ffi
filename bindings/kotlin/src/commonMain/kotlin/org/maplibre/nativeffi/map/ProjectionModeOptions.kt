package org.maplibre.nativeffi.map

/**
 * Mutable descriptor for axonometric map projection mode options.
 *
 * Compares and hashes by field value; [copy] returns an independent instance. Keep an instance
 * unmodified while it is a key in a hash-based collection.
 */
public class ProjectionModeOptions {
  public var axonometric: Boolean? = null

  public var xSkew: Double? = null

  public var ySkew: Double? = null

  /** Returns an independent copy of this descriptor with [block] applied to the copy. */
  public fun copy(block: ProjectionModeOptions.() -> Unit = {}): ProjectionModeOptions =
    ProjectionModeOptions()
      .also {
        it.axonometric = axonometric
        it.xSkew = xSkew
        it.ySkew = ySkew
      }
      .apply(block)

  private val fields: List<Any?>
    get() = listOf(axonometric, xSkew, ySkew)

  override fun equals(other: Any?): Boolean =
    other is ProjectionModeOptions && fields == other.fields

  override fun hashCode(): Int = fields.hashCode()
}
