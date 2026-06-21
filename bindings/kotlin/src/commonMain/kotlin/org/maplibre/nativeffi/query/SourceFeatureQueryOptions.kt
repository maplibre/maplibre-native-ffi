package org.maplibre.nativeffi.query

import org.maplibre.nativeffi.json.JsonValue

/** Mutable options for source feature queries. */
public class SourceFeatureQueryOptions {
  public var sourceLayerIds: List<String>? = null

  public var filter: JsonValue? = null
}
