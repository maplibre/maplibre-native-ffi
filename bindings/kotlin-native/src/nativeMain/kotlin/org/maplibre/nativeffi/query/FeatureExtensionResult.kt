package org.maplibre.nativeffi.query

import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.json.JsonValue

/** Copied result from a feature extension query. */
public sealed interface FeatureExtensionResult {
  public data class Value(public val value: JsonValue) : FeatureExtensionResult

  public data class FeatureCollection(public val features: List<Feature>) : FeatureExtensionResult

  public data class Unknown(public val rawType: Int) : FeatureExtensionResult
}
