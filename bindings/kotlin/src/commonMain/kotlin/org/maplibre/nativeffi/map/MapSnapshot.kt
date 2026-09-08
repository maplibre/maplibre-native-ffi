package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.runtime.RuntimeEventMask

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
  /**
   * True while the map is inside a gesture.
   *
   * A camera update whose [org.maplibre.nativeffi.camera.CameraUpdate.gesturePhase] is
   * [org.maplibre.nativeffi.camera.GesturePhase.BEGIN] or
   * [org.maplibre.nativeffi.camera.GesturePhase.UPDATE] sets it;
   * [org.maplibre.nativeffi.camera.GesturePhase.END] and
   * [org.maplibre.nativeffi.camera.GesturePhase.CANCEL] clear it.
   */
  public val gestureInProgress: Boolean,
  /**
   * The event mask this map was created with, or the one [MapHandle.setEventMask] last committed.
   *
   * A map queues only the [RuntimeEventMask.ALL_MAP_EVENTS] bits of it and ignores the rest, so
   * this value reads back exactly as the host wrote it.
   */
  public val eventMask: RuntimeEventMask,
  public val latestRenderUpdateGeneration: Long,
  public val tileOptions: TileOptions,
  public val bounds: BoundOptions,
  public val freeCameraOptions: FreeCameraOptions,
)
