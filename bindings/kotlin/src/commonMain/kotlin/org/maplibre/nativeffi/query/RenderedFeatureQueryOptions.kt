package org.maplibre.nativeffi.query

import org.maplibre.nativeffi.json.JsonValue

/**
 * Mutable options for rendered feature queries.
 *
 * Compares and hashes by field value; [copy] returns an independent instance. Keep an instance
 * unmodified while it is a key in a hash-based collection.
 */
public class RenderedFeatureQueryOptions {
  public var layerIds: List<String>? = null

  public var filter: JsonValue? = null

  /** Returns an independent copy of this descriptor with [block] applied to the copy. */
  public fun copy(block: RenderedFeatureQueryOptions.() -> Unit = {}): RenderedFeatureQueryOptions =
    RenderedFeatureQueryOptions()
      .also {
        it.layerIds = layerIds?.toList()
        it.filter = filter
      }
      .apply(block)

  private val fields: List<Any?>
    get() = listOf(layerIds, filter)

  override fun equals(other: Any?): Boolean =
    other is RenderedFeatureQueryOptions && fields == other.fields

  override fun hashCode(): Int = fields.hashCode()
}
