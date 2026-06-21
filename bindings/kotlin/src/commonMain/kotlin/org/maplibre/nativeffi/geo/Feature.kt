package org.maplibre.nativeffi.geo

import org.maplibre.nativeffi.json.JsonValue

/** Immutable GeoJSON feature descriptor. */
public class Feature(
  public val geometry: Geometry,
  properties: List<JsonValue.Member>,
  public val identifier: FeatureIdentifier,
) {
  public val properties: List<JsonValue.Member> = properties.toList()

  override fun equals(other: Any?): Boolean =
    other is Feature &&
      geometry == other.geometry &&
      properties == other.properties &&
      identifier == other.identifier

  override fun hashCode(): Int {
    var result = geometry.hashCode()
    result = 31 * result + properties.hashCode()
    result = 31 * result + identifier.hashCode()
    return result
  }

  override fun toString(): String =
    "Feature(geometry=$geometry, properties=$properties, identifier=$identifier)"
}
