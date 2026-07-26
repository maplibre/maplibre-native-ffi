package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.internal.status.Status

/**
 * Mutable descriptor used when creating a [MapHandle].
 *
 * Compares and hashes by field value; [copy] returns an independent instance. Keep an instance
 * unmodified while it is a key in a hash-based collection.
 */
public class MapOptions {
  public var width: Int? = null
    set(value) {
      value?.let { Status.requireArgument(it >= 0) { "width must be non-negative" } }
      field = value
    }

  public var height: Int? = null
    set(value) {
      value?.let { Status.requireArgument(it >= 0) { "height must be non-negative" } }
      field = value
    }

  public var scaleFactor: Double? = null

  public var mapMode: MapMode? = null

  /** Returns an independent copy of this descriptor with [block] applied to the copy. */
  public fun copy(block: MapOptions.() -> Unit = {}): MapOptions =
    MapOptions()
      .also {
        it.width = width
        it.height = height
        it.scaleFactor = scaleFactor
        it.mapMode = mapMode
      }
      .apply(block)

  private val fields: List<Any?>
    get() = listOf(width, height, scaleFactor, mapMode)

  override fun equals(other: Any?): Boolean = other is MapOptions && fields == other.fields

  override fun hashCode(): Int = fields.hashCode()
}
