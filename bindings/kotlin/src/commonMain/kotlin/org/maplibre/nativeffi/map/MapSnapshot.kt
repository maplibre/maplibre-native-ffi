package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.camera.CameraOptions

/** One immutable state generation published by the map worker. */
public data class MapSnapshot(
  public val generation: Long,
  public val camera: CameraOptions,
  public val size: MapSize,
  public val projectionMode: ProjectionModeOptions,
  public val viewportOptions: ViewportOptions,
  public val isLoading: Boolean,
  public val isFullyRendered: Boolean,
  public val repaintDemand: Boolean,
  public val latestRenderUpdateGeneration: Long,
)
