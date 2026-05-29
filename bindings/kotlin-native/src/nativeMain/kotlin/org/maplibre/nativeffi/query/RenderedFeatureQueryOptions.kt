package org.maplibre.nativeffi.query

import org.maplibre.nativeffi.json.JsonValue

/** Mutable options for rendered feature queries. */
public class RenderedFeatureQueryOptions {
  public var layerIds: List<String>? = null

  public var filter: JsonValue? = null
}
