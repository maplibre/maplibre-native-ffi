package org.maplibre.nativeffi.query

import org.maplibre.nativeffi.json.JsonValue

/** Mutable options for rendered feature queries. */
public class RenderedFeatureQueryOptions {
  public var layerIds: List<String>? = null
    private set

  public var filter: JsonValue? = null
    private set

  public fun hasLayerIds(): Boolean = layerIds != null

  public fun layerIds(layerIds: List<String>): RenderedFeatureQueryOptions = apply {
    this.layerIds = layerIds.toList()
  }

  public fun clearLayerIds(): RenderedFeatureQueryOptions = apply { layerIds = null }

  public fun hasFilter(): Boolean = filter != null

  public fun filter(filter: JsonValue): RenderedFeatureQueryOptions = apply { this.filter = filter }

  public fun clearFilter(): RenderedFeatureQueryOptions = apply { filter = null }
}
