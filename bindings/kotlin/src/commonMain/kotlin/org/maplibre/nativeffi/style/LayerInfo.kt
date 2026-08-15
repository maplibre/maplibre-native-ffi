package org.maplibre.nativeffi.style

/**
 * Copied metadata for one style layer.
 *
 * [sourceId] and [sourceLayer] are present when the layer carries them. The value remains valid
 * after its layer or map closes.
 */
public data class LayerInfo(
  /** Style-spec layer type string, for example `"fill"` or `"background"`. */
  public val type: String,
  /** Lowest zoom at which the layer draws; negative infinity with no lower bound. */
  public val minZoom: Double,
  /** Highest zoom at which the layer draws; positive infinity with no upper bound. */
  public val maxZoom: Double,
  public val visibility: StyleLayerVisibility,
  public val sourceId: String?,
  public val sourceLayer: String?,
)
