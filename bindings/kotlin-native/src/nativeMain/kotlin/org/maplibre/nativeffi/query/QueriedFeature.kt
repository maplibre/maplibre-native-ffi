package org.maplibre.nativeffi.query

import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.json.JsonValue

/** One feature copied from a rendered or source feature query result. */
public data class QueriedFeature(
  public val feature: Feature,
  public val sourceId: String? = null,
  public val sourceLayerId: String? = null,
  public val state: JsonValue? = null,
)
