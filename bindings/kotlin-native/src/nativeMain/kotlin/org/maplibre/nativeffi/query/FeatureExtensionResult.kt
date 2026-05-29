package org.maplibre.nativeffi.query

import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.json.JsonValue

/** Copied result from a feature extension query. */
public sealed interface FeatureExtensionResult {
  public data class Value(public val value: JsonValue) : FeatureExtensionResult

  public class FeatureCollection(features: List<Feature>) : FeatureExtensionResult {
    public val features: List<Feature> = features.toList()

    override fun equals(other: Any?): Boolean =
      other is FeatureCollection && features == other.features

    override fun hashCode(): Int = features.hashCode()

    override fun toString(): String = "FeatureCollection(features=$features)"
  }

  public data class Unknown(public val rawType: Int) : FeatureExtensionResult
}
