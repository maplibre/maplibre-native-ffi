package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.internal.status.Status

/**
 * Mutable descriptor for tile prefetch and level-of-detail controls.
 *
 * Compares and hashes by field value; [copy] returns an independent instance. Keep an instance
 * unmodified while it is a key in a hash-based collection.
 */
public class TileOptions {
  public var prefetchZoomDelta: Int? = null
    set(value) {
      value?.let { Status.requireArgument(it >= 0) { "prefetchZoomDelta must be non-negative" } }
      field = value
    }

  public var lodMinRadius: Double? = null

  public var lodScale: Double? = null

  public var lodPitchThreshold: Double? = null

  public var lodZoomShift: Double? = null

  public var lodMode: TileLodMode? = null

  /** Returns an independent copy of this descriptor with [block] applied to the copy. */
  public fun copy(block: TileOptions.() -> Unit = {}): TileOptions =
    TileOptions()
      .also {
        it.prefetchZoomDelta = prefetchZoomDelta
        it.lodMinRadius = lodMinRadius
        it.lodScale = lodScale
        it.lodPitchThreshold = lodPitchThreshold
        it.lodZoomShift = lodZoomShift
        it.lodMode = lodMode
      }
      .apply(block)

  private val fields: List<Any?>
    get() =
      listOf(prefetchZoomDelta, lodMinRadius, lodScale, lodPitchThreshold, lodZoomShift, lodMode)

  override fun equals(other: Any?): Boolean = other is TileOptions && fields == other.fields

  override fun hashCode(): Int = fields.hashCode()
}
