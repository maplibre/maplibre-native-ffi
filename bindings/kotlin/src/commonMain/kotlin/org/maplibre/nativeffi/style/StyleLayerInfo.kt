package org.maplibre.nativeffi.style

/**
 * Copied description of one style layer.
 *
 * [type] is the style-spec layer type, such as `line`. [sourceId] is absent when the layer type
 * takes no source, and [sourceLayer] is absent when the layer names none. The value remains valid
 * after later style changes and after its map closes.
 */
public data class StyleLayerInfo(
  public val id: String,
  public val type: String,
  public val sourceId: String?,
  public val sourceLayer: String?,
)
