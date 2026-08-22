package org.maplibre.nativeffi.style

/** Mutable descriptor for custom MVT vector sources. */
public class CustomMvtVectorSourceOptions(public val callback: CustomMvtVectorSourceCallback) {
  public var minZoom: Double? = null

  public var maxZoom: Double? = null
}
