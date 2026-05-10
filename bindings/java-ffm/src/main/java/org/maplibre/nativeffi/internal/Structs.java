package org.maplibre.nativeffi.internal;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import org.maplibre.nativeffi.EdgeInsets;
import org.maplibre.nativeffi.LatLng;
import org.maplibre.nativeffi.MapOptions;
import org.maplibre.nativeffi.OfflineRegionDownloadState;
import org.maplibre.nativeffi.OfflineRegionStatus;
import org.maplibre.nativeffi.ProjectedMeters;
import org.maplibre.nativeffi.RenderingStats;
import org.maplibre.nativeffi.RuntimeOptions;
import org.maplibre.nativeffi.ScreenPoint;
import org.maplibre.nativeffi.TileId;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_edge_insets;
import org.maplibre.nativeffi.internal.c.mln_lat_lng;
import org.maplibre.nativeffi.internal.c.mln_map_options;
import org.maplibre.nativeffi.internal.c.mln_offline_region_status;
import org.maplibre.nativeffi.internal.c.mln_projected_meters;
import org.maplibre.nativeffi.internal.c.mln_rendering_stats;
import org.maplibre.nativeffi.internal.c.mln_runtime_options;
import org.maplibre.nativeffi.internal.c.mln_screen_point;
import org.maplibre.nativeffi.internal.c.mln_tile_id;

/** Internal struct materializers and readers. */
public final class Structs {
  private Structs() {}

  public static MemorySegment runtimeOptions(RuntimeOptions options, Arena arena) {
    var segment = MapLibreNativeC.mln_runtime_options_default(arena);
    if (options.assetPath() != null) {
      mln_runtime_options.asset_path(
          segment, MemoryUtil.allocateCString(arena, options.assetPath()));
    }
    if (options.cachePath() != null) {
      mln_runtime_options.cache_path(
          segment, MemoryUtil.allocateCString(arena, options.cachePath()));
    }
    if (options.maximumCacheSize().isPresent()) {
      mln_runtime_options.flags(
          segment,
          mln_runtime_options.flags(segment)
              | MapLibreNativeC.MLN_RUNTIME_OPTION_MAXIMUM_CACHE_SIZE());
      mln_runtime_options.maximum_cache_size(segment, options.maximumCacheSize().getAsLong());
    }
    return segment;
  }

  public static MemorySegment mapOptions(MapOptions options, Arena arena) {
    var segment = MapLibreNativeC.mln_map_options_default(arena);
    if (options.width() != null) {
      mln_map_options.width(segment, options.width());
    }
    if (options.height() != null) {
      mln_map_options.height(segment, options.height());
    }
    if (options.scaleFactor() != null) {
      mln_map_options.scale_factor(segment, options.scaleFactor());
    }
    if (options.mapMode() != null) {
      mln_map_options.map_mode(segment, options.mapMode().nativeValue());
    }
    return segment;
  }

  public static MemorySegment latLng(LatLng coordinate, Arena arena) {
    var segment = mln_lat_lng.allocate(arena);
    mln_lat_lng.latitude(segment, coordinate.latitude());
    mln_lat_lng.longitude(segment, coordinate.longitude());
    return segment;
  }

  public static LatLng latLng(MemorySegment segment) {
    return new LatLng(mln_lat_lng.latitude(segment), mln_lat_lng.longitude(segment));
  }

  public static MemorySegment screenPoint(ScreenPoint point, Arena arena) {
    var segment = mln_screen_point.allocate(arena);
    mln_screen_point.x(segment, point.x());
    mln_screen_point.y(segment, point.y());
    return segment;
  }

  public static ScreenPoint screenPoint(MemorySegment segment) {
    return new ScreenPoint(mln_screen_point.x(segment), mln_screen_point.y(segment));
  }

  public static MemorySegment projectedMeters(ProjectedMeters meters, Arena arena) {
    var segment = mln_projected_meters.allocate(arena);
    mln_projected_meters.northing(segment, meters.northing());
    mln_projected_meters.easting(segment, meters.easting());
    return segment;
  }

  public static ProjectedMeters projectedMeters(MemorySegment segment) {
    return new ProjectedMeters(
        mln_projected_meters.northing(segment), mln_projected_meters.easting(segment));
  }

  public static MemorySegment edgeInsets(EdgeInsets insets, Arena arena) {
    var segment = mln_edge_insets.allocate(arena);
    mln_edge_insets.top(segment, insets.top());
    mln_edge_insets.left(segment, insets.left());
    mln_edge_insets.bottom(segment, insets.bottom());
    mln_edge_insets.right(segment, insets.right());
    return segment;
  }

  public static RenderingStats renderingStats(MemorySegment segment) {
    return new RenderingStats(
        mln_rendering_stats.encoding_time(segment),
        mln_rendering_stats.rendering_time(segment),
        mln_rendering_stats.frame_count(segment),
        mln_rendering_stats.draw_call_count(segment),
        mln_rendering_stats.total_draw_call_count(segment));
  }

  public static TileId tileId(MemorySegment segment) {
    return new TileId(
        Integer.toUnsignedLong(mln_tile_id.overscaled_z(segment)),
        mln_tile_id.wrap(segment),
        Integer.toUnsignedLong(mln_tile_id.canonical_z(segment)),
        Integer.toUnsignedLong(mln_tile_id.canonical_x(segment)),
        Integer.toUnsignedLong(mln_tile_id.canonical_y(segment)));
  }

  public static OfflineRegionStatus offlineRegionStatus(MemorySegment segment) {
    var rawDownloadState = mln_offline_region_status.download_state(segment);
    return new OfflineRegionStatus(
        OfflineRegionDownloadState.fromNative(rawDownloadState),
        rawDownloadState,
        mln_offline_region_status.completed_resource_count(segment),
        mln_offline_region_status.completed_resource_size(segment),
        mln_offline_region_status.completed_tile_count(segment),
        mln_offline_region_status.required_tile_count(segment),
        mln_offline_region_status.completed_tile_size(segment),
        mln_offline_region_status.required_resource_count(segment),
        mln_offline_region_status.required_resource_count_is_precise(segment),
        mln_offline_region_status.complete(segment));
  }
}
