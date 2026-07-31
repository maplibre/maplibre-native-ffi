package org.maplibre.nativeffi.style

/**
 * Mutable descriptor for runtime style image options.
 *
 * Compares and hashes by field value; [copy] returns an independent instance. Assigning [stretchX]
 * or [stretchY] snapshots the caller's list, so later caller mutation does not change this
 * descriptor. Keep an instance unmodified while it is a key in a hash-based collection.
 */
public class StyleImageOptions {
  public var pixelRatio: Float? = null

  public var sdf: Boolean? = null

  /**
   * Horizontally stretchable intervals. A present empty list stays distinguishable from an absent
   * one.
   */
  public var stretchX: List<ImageStretch>? = null
    set(value) {
      field = value?.toList()
    }

  /** Vertically stretchable intervals. */
  public var stretchY: List<ImageStretch>? = null
    set(value) {
      field = value?.toList()
    }

  /** Content box used when `icon-text-fit` applies. */
  public var content: ImageContent? = null

  public var textFitWidth: StyleImageTextFit? = null

  public var textFitHeight: StyleImageTextFit? = null

  /** Returns an independent copy of this descriptor with [block] applied to the copy. */
  public fun copy(block: StyleImageOptions.() -> Unit = {}): StyleImageOptions =
    StyleImageOptions()
      .also {
        it.pixelRatio = pixelRatio
        it.sdf = sdf
        it.stretchX = stretchX
        it.stretchY = stretchY
        it.content = content
        it.textFitWidth = textFitWidth
        it.textFitHeight = textFitHeight
      }
      .apply(block)

  private val fields: List<Any?>
    get() = listOf(pixelRatio, sdf, stretchX, stretchY, content, textFitWidth, textFitHeight)

  override fun equals(other: Any?): Boolean = other is StyleImageOptions && fields == other.fields

  override fun hashCode(): Int = fields.hashCode()
}
