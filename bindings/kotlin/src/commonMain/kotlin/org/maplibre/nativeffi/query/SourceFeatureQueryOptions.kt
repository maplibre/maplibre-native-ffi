package org.maplibre.nativeffi.query

import org.maplibre.nativeffi.json.JsonValue

/**
 * Mutable options for source feature queries.
 *
 * Compares and hashes by field value; [copy] returns an independent instance. Assigning
 * [sourceLayerIds] snapshots the caller's list, so later caller mutation does not change this
 * descriptor. Keep an instance unmodified while it is a key in a hash-based collection.
 */
public class SourceFeatureQueryOptions {
  public var sourceLayerIds: List<String>? = null
    set(value) {
      field = value?.toList()
    }

  public var filter: JsonValue? = null

  /** Returns an independent copy of this descriptor with [block] applied to the copy. */
  public fun copy(block: SourceFeatureQueryOptions.() -> Unit = {}): SourceFeatureQueryOptions =
    SourceFeatureQueryOptions()
      .also {
        it.sourceLayerIds = sourceLayerIds
        it.filter = filter
      }
      .apply(block)

  private val fields: List<Any?>
    get() = listOf(sourceLayerIds, filter)

  override fun equals(other: Any?): Boolean =
    other is SourceFeatureQueryOptions && fields == other.fields

  override fun hashCode(): Int = fields.hashCode()
}
