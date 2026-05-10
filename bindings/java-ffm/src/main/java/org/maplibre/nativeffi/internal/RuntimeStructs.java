package org.maplibre.nativeffi.internal;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import org.maplibre.nativeffi.OfflineRegionDownloadState;
import org.maplibre.nativeffi.OfflineRegionStatus;
import org.maplibre.nativeffi.RenderingStats;
import org.maplibre.nativeffi.RuntimeOptions;
import org.maplibre.nativeffi.TileId;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_offline_region_status;
import org.maplibre.nativeffi.internal.c.mln_rendering_stats;
import org.maplibre.nativeffi.internal.c.mln_runtime_options;
import org.maplibre.nativeffi.internal.c.mln_tile_id;

/** Internal materializers and readers for runtime structs and runtime event payloads. */
public final class RuntimeStructs {
  private RuntimeStructs() {}

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
