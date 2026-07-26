package org.maplibre.nativeffi.camera

import org.maplibre.nativeffi.geo.LatLngBounds

/**
 * Mutable map bounds descriptor.
 *
 * Compares and hashes by field value; [copy] returns an independent instance. Keep an instance
 * unmodified while it is a key in a hash-based collection.
 */
public class BoundOptions {
  public var bounds: LatLngBounds? = null

  public var minZoom: Double? = null

  public var maxZoom: Double? = null

  public var minPitch: Double? = null

  public var maxPitch: Double? = null

  /** Returns an independent copy of this descriptor with [block] applied to the copy. */
  public fun copy(block: BoundOptions.() -> Unit = {}): BoundOptions =
    BoundOptions()
      .also {
        it.bounds = bounds
        it.minZoom = minZoom
        it.maxZoom = maxZoom
        it.minPitch = minPitch
        it.maxPitch = maxPitch
      }
      .apply(block)

  private val fields: List<Any?>
    get() = listOf(bounds, minZoom, maxZoom, minPitch, maxPitch)

  override fun equals(other: Any?): Boolean = other is BoundOptions && fields == other.fields

  override fun hashCode(): Int = fields.hashCode()
}
