package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.runtime.RuntimeEventMask
import org.maplibre.nativeffi.runtime.defaultMapEventMask

/**
 * Mutable descriptor used when creating a [MapHandle].
 *
 * Compares and hashes by field value; [copy] returns an independent instance. Keep an instance
 * unmodified while it is a key in a hash-based collection.
 */
public class MapOptions {
  /**
   * Initial logical width in UI pixels, replaced by the extent of the first attached render
   * session.
   */
  public var width: Int? = null
    set(value) {
      value?.let { Status.requireArgument(it >= 0) { "width must be non-negative" } }
      field = value
    }

  /**
   * Initial logical height in UI pixels, replaced by the extent of the first attached render
   * session.
   */
  public var height: Int? = null
    set(value) {
      value?.let { Status.requireArgument(it >= 0) { "height must be non-negative" } }
      field = value
    }

  /**
   * UI-to-device pixel scale, fixed for the lifetime of the map. It selects sprites, glyphs, and
   * raster tiles; render targets carry their own scale factor for geometry.
   */
  public var scaleFactor: Double? = null

  public var mapMode: MapMode? = null

  /**
   * Decodes MapLibre Tile (MLT) tiles whose integer streams use FastPFOR encodings, fixed for the
   * lifetime of the map. A map created with this `false` logs a tile parse warning for such tiles.
   */
  public var fastPforEnabled: Boolean? = null

  /**
   * Map-originated event types this map queues, the native library's default until a host narrows
   * it. That default selects every map-originated type the library reports, including a type this
   * version does not name, whose events reach a host as unknown event and payload domains.
   *
   * [MapHandle.create] fails with [org.maplibre.nativeffi.error.InvalidArgumentException] on a bit
   * the native library does not define.
   */
  public var eventMask: RuntimeEventMask = defaultMapEventMask()

  /** Returns an independent copy of this descriptor with [block] applied to the copy. */
  public fun copy(block: MapOptions.() -> Unit = {}): MapOptions =
    MapOptions()
      .also {
        it.width = width
        it.height = height
        it.scaleFactor = scaleFactor
        it.mapMode = mapMode
        it.fastPforEnabled = fastPforEnabled
        it.eventMask = eventMask
      }
      .apply(block)

  private val fields: List<Any?>
    get() = listOf(width, height, scaleFactor, mapMode, fastPforEnabled, eventMask)

  override fun equals(other: Any?): Boolean = other is MapOptions && fields == other.fields

  override fun hashCode(): Int = fields.hashCode()
}
