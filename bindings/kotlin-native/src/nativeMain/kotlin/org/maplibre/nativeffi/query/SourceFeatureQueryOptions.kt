package org.maplibre.nativeffi.query

import org.maplibre.nativeffi.json.JsonValue

/** Mutable options for source feature queries. */
public class SourceFeatureQueryOptions {
  public var sourceLayerIds: List<String>? = null
    private set

  public var filter: JsonValue? = null
    private set

  public fun hasSourceLayerIds(): Boolean = sourceLayerIds != null

  public fun sourceLayerIds(sourceLayerIds: List<String>): SourceFeatureQueryOptions = apply {
    this.sourceLayerIds = sourceLayerIds.toList()
  }

  public fun clearSourceLayerIds(): SourceFeatureQueryOptions = apply { sourceLayerIds = null }

  public fun hasFilter(): Boolean = filter != null

  public fun filter(filter: JsonValue): SourceFeatureQueryOptions = apply { this.filter = filter }

  public fun clearFilter(): SourceFeatureQueryOptions = apply { filter = null }
}
