package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.FreeCameraOptions

/**
 * One immutable state generation published by the map worker.
 *
 * Every committed map command publishes a new generation in its completion, so a snapshot whose
 * [generation] is at or past that value observes the commit.
 */
public data class MapSnapshot(
  public val generation: Long,
  public val debugOptions: Set<DebugOption>,
  public val camera: CameraOptions,
  public val size: MapSize,
  public val projectionMode: ProjectionModeOptions,
  public val viewportOptions: ViewportOptions,
  /** True once every requested style and tile resource finished loading. */
  public val isFullyLoaded: Boolean,
  public val renderingStatsViewEnabled: Boolean,
  public val repaintDemand: Boolean,
  public val latestRenderUpdateGeneration: Long,
  public val tileOptions: TileOptions,
  public val bounds: BoundOptions,
  public val freeCameraOptions: FreeCameraOptions,
)
