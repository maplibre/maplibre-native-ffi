package org.maplibre.nativeffi.style

/**
 * Mutable descriptor for runtime style image options.
 *
 * Compares and hashes by field value; [copy] returns an independent instance. Keep an instance
 * unmodified while it is a key in a hash-based collection.
 */
public class StyleImageOptions {
  public var pixelRatio: Float? = null

  public var sdf: Boolean? = null

  /** Returns an independent copy of this descriptor with [block] applied to the copy. */
  public fun copy(block: StyleImageOptions.() -> Unit = {}): StyleImageOptions =
    StyleImageOptions()
      .also {
        it.pixelRatio = pixelRatio
        it.sdf = sdf
      }
      .apply(block)

  private val fields: List<Any?>
    get() = listOf(pixelRatio, sdf)

  override fun equals(other: Any?): Boolean = other is StyleImageOptions && fields == other.fields

  override fun hashCode(): Int = fields.hashCode()
}
