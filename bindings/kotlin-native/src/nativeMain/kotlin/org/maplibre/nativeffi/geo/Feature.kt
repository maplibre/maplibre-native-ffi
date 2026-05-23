package org.maplibre.nativeffi.geo

import org.maplibre.nativeffi.json.JsonValue

/** Immutable GeoJSON feature descriptor. */
public data class Feature(
  public val geometry: Geometry,
  public val properties: List<JsonValue.Member>,
  public val identifier: FeatureIdentifier = FeatureIdentifier.Null,
)
