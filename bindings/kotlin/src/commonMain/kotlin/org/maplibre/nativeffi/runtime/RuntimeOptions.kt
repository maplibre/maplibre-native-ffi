package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.internal.status.Status

/**
 * Mutable descriptor used when creating a [RuntimeHandle].
 *
 * Compares and hashes by field value; [copy] returns an independent instance. Keep an instance
 * unmodified while it is a key in a hash-based collection.
 */
public class RuntimeOptions {
  public var assetPath: String? = null

  public var cachePath: String? = null

  public var maximumCacheSize: Long? = null
    set(value) {
      value?.let { Status.requireArgument(it >= 0) { "maximumCacheSize must be non-negative" } }
      field = value
    }

  /** Returns an independent copy of this descriptor with [block] applied to the copy. */
  public fun copy(block: RuntimeOptions.() -> Unit = {}): RuntimeOptions =
    RuntimeOptions()
      .also {
        it.assetPath = assetPath
        it.cachePath = cachePath
        it.maximumCacheSize = maximumCacheSize
      }
      .apply(block)

  private val fields: List<Any?>
    get() = listOf(assetPath, cachePath, maximumCacheSize)

  override fun equals(other: Any?): Boolean = other is RuntimeOptions && fields == other.fields

  override fun hashCode(): Int = fields.hashCode()
}
