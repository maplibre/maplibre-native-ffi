package org.maplibre.nativeffi.internal.struct;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.util.ArrayList;
import java.util.List;
import org.maplibre.nativeffi.geo.TileId;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_offline_geometry_region_definition;
import org.maplibre.nativeffi.internal.c.mln_offline_region_definition;
import org.maplibre.nativeffi.internal.c.mln_offline_region_info;
import org.maplibre.nativeffi.internal.c.mln_offline_region_status;
import org.maplibre.nativeffi.internal.c.mln_offline_tile_pyramid_region_definition;
import org.maplibre.nativeffi.internal.c.mln_rendering_stats;
import org.maplibre.nativeffi.internal.c.mln_runtime_options;
import org.maplibre.nativeffi.internal.c.mln_tile_id;
import org.maplibre.nativeffi.internal.convert.NativeValues;
import org.maplibre.nativeffi.internal.memory.MemoryUtil;
import org.maplibre.nativeffi.internal.status.Status;
import org.maplibre.nativeffi.map.RenderingStats;
import org.maplibre.nativeffi.offline.OfflineRegionDefinition;
import org.maplibre.nativeffi.offline.OfflineRegionInfo;
import org.maplibre.nativeffi.offline.OfflineRegionStatus;
import org.maplibre.nativeffi.runtime.RuntimeOptions;

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

  public static MemorySegment offlineRegionStatus(Arena arena) {
    var segment = mln_offline_region_status.allocate(arena);
    mln_offline_region_status.size(segment, (int) mln_offline_region_status.sizeof());
    return segment;
  }

  public static OfflineRegionStatus offlineRegionStatus(MemorySegment segment) {
    var rawDownloadState = mln_offline_region_status.download_state(segment);
    return new OfflineRegionStatus(
        NativeValues.offlineRegionDownloadState(rawDownloadState),
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

  public static MemorySegment offlineRegionDefinition(
      OfflineRegionDefinition definition, Arena arena) {
    var segment = mln_offline_region_definition.allocate(arena);
    mln_offline_region_definition.size(segment, (int) mln_offline_region_definition.sizeof());
    var data = mln_offline_region_definition.data(segment);
    if (definition instanceof OfflineRegionDefinition.TilePyramid tilePyramid) {
      mln_offline_region_definition.type(
          segment, MapLibreNativeC.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID());
      var nativeTile = mln_offline_region_definition.data.tile_pyramid(data);
      mln_offline_tile_pyramid_region_definition.size(
          nativeTile, (int) mln_offline_tile_pyramid_region_definition.sizeof());
      mln_offline_tile_pyramid_region_definition.style_url(
          nativeTile, MemoryUtil.allocateCString(arena, tilePyramid.styleUrl()));
      mln_offline_tile_pyramid_region_definition.bounds(
          nativeTile, CoreStructs.latLngBounds(tilePyramid.bounds(), arena));
      mln_offline_tile_pyramid_region_definition.min_zoom(nativeTile, tilePyramid.minZoom());
      mln_offline_tile_pyramid_region_definition.max_zoom(nativeTile, tilePyramid.maxZoom());
      mln_offline_tile_pyramid_region_definition.pixel_ratio(nativeTile, tilePyramid.pixelRatio());
      mln_offline_tile_pyramid_region_definition.include_ideographs(
          nativeTile, tilePyramid.includeIdeographs());
      return segment;
    }
    if (definition instanceof OfflineRegionDefinition.GeometryRegion geometryRegion) {
      mln_offline_region_definition.type(
          segment, MapLibreNativeC.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY());
      var nativeGeometry = mln_offline_region_definition.data.geometry(data);
      mln_offline_geometry_region_definition.size(
          nativeGeometry, (int) mln_offline_geometry_region_definition.sizeof());
      mln_offline_geometry_region_definition.style_url(
          nativeGeometry, MemoryUtil.allocateCString(arena, geometryRegion.styleUrl()));
      mln_offline_geometry_region_definition.geometry(
          nativeGeometry, ValueStructs.geometry(geometryRegion.geometry(), arena));
      mln_offline_geometry_region_definition.min_zoom(nativeGeometry, geometryRegion.minZoom());
      mln_offline_geometry_region_definition.max_zoom(nativeGeometry, geometryRegion.maxZoom());
      mln_offline_geometry_region_definition.pixel_ratio(
          nativeGeometry, geometryRegion.pixelRatio());
      mln_offline_geometry_region_definition.include_ideographs(
          nativeGeometry, geometryRegion.includeIdeographs());
      return segment;
    }
    throw new IllegalArgumentException(
        "Unsupported offline region definition type: " + definition.getClass().getName());
  }

  public static MemorySegment metadata(byte[] metadata, Arena arena) {
    if (metadata.length == 0) {
      return MemorySegment.NULL;
    }
    var segment = arena.allocate(metadata.length);
    MemorySegment.copy(
        metadata, 0, segment, java.lang.foreign.ValueLayout.JAVA_BYTE, 0, metadata.length);
    return segment;
  }

  public static OfflineRegionInfo offlineRegionInfo(MemorySegment segment) {
    return new OfflineRegionInfo(
        mln_offline_region_info.id(segment),
        offlineRegionDefinition(mln_offline_region_info.definition(segment)),
        MemoryUtil.copyBytes(
            mln_offline_region_info.metadata(segment),
            mln_offline_region_info.metadata_size(segment)));
  }

  public static OfflineRegionDefinition offlineRegionDefinition(MemorySegment segment) {
    var type = mln_offline_region_definition.type(segment);
    var data = mln_offline_region_definition.data(segment);
    if (type == MapLibreNativeC.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID()) {
      var tile = mln_offline_region_definition.data.tile_pyramid(data);
      return new OfflineRegionDefinition.TilePyramid(
          MemoryUtil.copyCString(mln_offline_tile_pyramid_region_definition.style_url(tile)),
          CoreStructs.latLngBounds(mln_offline_tile_pyramid_region_definition.bounds(tile)),
          mln_offline_tile_pyramid_region_definition.min_zoom(tile),
          mln_offline_tile_pyramid_region_definition.max_zoom(tile),
          mln_offline_tile_pyramid_region_definition.pixel_ratio(tile),
          mln_offline_tile_pyramid_region_definition.include_ideographs(tile));
    }
    if (type == MapLibreNativeC.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY()) {
      var geometry = mln_offline_region_definition.data.geometry(data);
      return new OfflineRegionDefinition.GeometryRegion(
          MemoryUtil.copyCString(mln_offline_geometry_region_definition.style_url(geometry)),
          ValueStructs.geometry(mln_offline_geometry_region_definition.geometry(geometry)),
          mln_offline_geometry_region_definition.min_zoom(geometry),
          mln_offline_geometry_region_definition.max_zoom(geometry),
          mln_offline_geometry_region_definition.pixel_ratio(geometry),
          mln_offline_geometry_region_definition.include_ideographs(geometry));
    }
    throw new IllegalArgumentException("Unknown offline region definition type: " + type);
  }

  public static OfflineRegionInfo offlineRegionSnapshot(MemorySegment snapshot) {
    try (var arena = Arena.ofConfined()) {
      var info = mln_offline_region_info.allocate(arena);
      mln_offline_region_info.size(info, (int) mln_offline_region_info.sizeof());
      Status.check(MapLibreNativeC.mln_offline_region_snapshot_get(snapshot, info));
      return offlineRegionInfo(info);
    } finally {
      MapLibreNativeC.mln_offline_region_snapshot_destroy(snapshot);
    }
  }

  public static List<OfflineRegionInfo> offlineRegionList(MemorySegment list) {
    try (var arena = Arena.ofConfined()) {
      var outCount = arena.allocate(java.lang.foreign.ValueLayout.JAVA_LONG);
      Status.check(MapLibreNativeC.mln_offline_region_list_count(list, outCount));
      var count = Math.toIntExact(outCount.get(java.lang.foreign.ValueLayout.JAVA_LONG, 0));
      var regions = new ArrayList<OfflineRegionInfo>(count);
      for (var index = 0; index < count; index++) {
        var info = mln_offline_region_info.allocate(arena);
        mln_offline_region_info.size(info, (int) mln_offline_region_info.sizeof());
        Status.check(MapLibreNativeC.mln_offline_region_list_get(list, index, info));
        regions.add(offlineRegionInfo(info));
      }
      return List.copyOf(regions);
    } finally {
      MapLibreNativeC.mln_offline_region_list_destroy(list);
    }
  }
}
