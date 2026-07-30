package org.maplibre.nativeffi.runtime

/**
 * Mutable descriptor used when creating a [RuntimeHandle].
 *
 * Compares and hashes by field value; [copy] returns an independent instance. Keep an instance
 * unmodified while it is a key in a hash-based collection.
 */
public class RuntimeOptions {
  public var assetPath: String? = null

  public var cachePath: String? = null

  /** Returns an independent copy of this descriptor with [block] applied to the copy. */
  public fun copy(block: RuntimeOptions.() -> Unit = {}): RuntimeOptions =
    RuntimeOptions()
      .also {
        it.assetPath = assetPath
        it.cachePath = cachePath
      }
      .apply(block)

  private val fields: List<Any?>
    get() = listOf(assetPath, cachePath)

  override fun equals(other: Any?): Boolean = other is RuntimeOptions && fields == other.fields

  override fun hashCode(): Int = fields.hashCode()
}
