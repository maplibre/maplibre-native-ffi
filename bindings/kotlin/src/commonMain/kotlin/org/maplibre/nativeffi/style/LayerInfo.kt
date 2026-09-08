package org.maplibre.nativeffi.style

/**
 * Copied metadata for one style layer.
 *
 * The value remains valid after its layer or map closes.
 */
public data class LayerInfo(
  /** Style-spec layer type string, for example `"fill"` or `"background"`. */
  public val type: String,
  /** Lowest zoom at which the layer draws; negative infinity with no lower bound. */
  public val minZoom: Double,
  /** Highest zoom at which the layer draws; positive infinity with no upper bound. */
  public val maxZoom: Double,
  public val visibility: StyleLayerVisibility,
  /** Source ID, null for a layer type that takes no source. */
  public val sourceId: String?,
  /** Source-layer ID, null when the layer sets none. */
  public val sourceLayer: String?,
)
