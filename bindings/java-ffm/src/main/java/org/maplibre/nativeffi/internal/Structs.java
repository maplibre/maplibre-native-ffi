package org.maplibre.nativeffi.internal;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import org.maplibre.nativeffi.AnimationOptions;
import org.maplibre.nativeffi.BoundOptions;
import org.maplibre.nativeffi.CameraFitOptions;
import org.maplibre.nativeffi.CameraOptions;
import org.maplibre.nativeffi.ConstrainMode;
import org.maplibre.nativeffi.EdgeInsets;
import org.maplibre.nativeffi.Feature;
import org.maplibre.nativeffi.FeatureIdentifier;
import org.maplibre.nativeffi.FreeCameraOptions;
import org.maplibre.nativeffi.GeoJson;
import org.maplibre.nativeffi.Geometry;
import org.maplibre.nativeffi.JsonValue;
import org.maplibre.nativeffi.LatLng;
import org.maplibre.nativeffi.LatLngBounds;
import org.maplibre.nativeffi.MapOptions;
import org.maplibre.nativeffi.MapTileOptions;
import org.maplibre.nativeffi.MapViewportOptions;
import org.maplibre.nativeffi.NorthOrientation;
import org.maplibre.nativeffi.OfflineRegionDownloadState;
import org.maplibre.nativeffi.OfflineRegionStatus;
import org.maplibre.nativeffi.ProjectedMeters;
import org.maplibre.nativeffi.ProjectionModeOptions;
import org.maplibre.nativeffi.Quaternion;
import org.maplibre.nativeffi.RenderingStats;
import org.maplibre.nativeffi.ResourceKind;
import org.maplibre.nativeffi.ResourceLoadingMethod;
import org.maplibre.nativeffi.ResourcePriority;
import org.maplibre.nativeffi.ResourceRequest;
import org.maplibre.nativeffi.ResourceResponse;
import org.maplibre.nativeffi.ResourceStoragePolicy;
import org.maplibre.nativeffi.ResourceUsage;
import org.maplibre.nativeffi.RuntimeOptions;
import org.maplibre.nativeffi.ScreenPoint;
import org.maplibre.nativeffi.TileId;
import org.maplibre.nativeffi.TileLodMode;
import org.maplibre.nativeffi.UnitBezier;
import org.maplibre.nativeffi.Vec3;
import org.maplibre.nativeffi.ViewportMode;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_animation_options;
import org.maplibre.nativeffi.internal.c.mln_bound_options;
import org.maplibre.nativeffi.internal.c.mln_camera_fit_options;
import org.maplibre.nativeffi.internal.c.mln_camera_options;
import org.maplibre.nativeffi.internal.c.mln_coordinate_span;
import org.maplibre.nativeffi.internal.c.mln_edge_insets;
import org.maplibre.nativeffi.internal.c.mln_feature;
import org.maplibre.nativeffi.internal.c.mln_feature_collection;
import org.maplibre.nativeffi.internal.c.mln_free_camera_options;
import org.maplibre.nativeffi.internal.c.mln_geojson;
import org.maplibre.nativeffi.internal.c.mln_geometry;
import org.maplibre.nativeffi.internal.c.mln_geometry_collection;
import org.maplibre.nativeffi.internal.c.mln_json_array;
import org.maplibre.nativeffi.internal.c.mln_json_member;
import org.maplibre.nativeffi.internal.c.mln_json_object;
import org.maplibre.nativeffi.internal.c.mln_json_value;
import org.maplibre.nativeffi.internal.c.mln_lat_lng;
import org.maplibre.nativeffi.internal.c.mln_lat_lng_bounds;
import org.maplibre.nativeffi.internal.c.mln_map_options;
import org.maplibre.nativeffi.internal.c.mln_map_tile_options;
import org.maplibre.nativeffi.internal.c.mln_map_viewport_options;
import org.maplibre.nativeffi.internal.c.mln_multi_line_geometry;
import org.maplibre.nativeffi.internal.c.mln_multi_polygon_geometry;
import org.maplibre.nativeffi.internal.c.mln_offline_region_status;
import org.maplibre.nativeffi.internal.c.mln_polygon_geometry;
import org.maplibre.nativeffi.internal.c.mln_projected_meters;
import org.maplibre.nativeffi.internal.c.mln_projection_mode;
import org.maplibre.nativeffi.internal.c.mln_quaternion;
import org.maplibre.nativeffi.internal.c.mln_rendering_stats;
import org.maplibre.nativeffi.internal.c.mln_resource_request;
import org.maplibre.nativeffi.internal.c.mln_resource_response;
import org.maplibre.nativeffi.internal.c.mln_runtime_options;
import org.maplibre.nativeffi.internal.c.mln_screen_point;
import org.maplibre.nativeffi.internal.c.mln_string_view;
import org.maplibre.nativeffi.internal.c.mln_tile_id;
import org.maplibre.nativeffi.internal.c.mln_unit_bezier;
import org.maplibre.nativeffi.internal.c.mln_vec3;

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

  public static MemorySegment cameraOptions(CameraOptions options, Arena arena) {
    var segment = MapLibreNativeC.mln_camera_options_default(arena);
    var fields = 0;
    if (options.hasCenter()) {
      fields |= MapLibreNativeC.MLN_CAMERA_OPTION_CENTER();
      mln_camera_options.latitude(segment, options.center().latitude());
      mln_camera_options.longitude(segment, options.center().longitude());
    }
    if (options.hasCenterAltitude()) {
      fields |= MapLibreNativeC.MLN_CAMERA_OPTION_CENTER_ALTITUDE();
      mln_camera_options.center_altitude(segment, options.centerAltitude());
    }
    if (options.hasPadding()) {
      fields |= MapLibreNativeC.MLN_CAMERA_OPTION_PADDING();
      mln_camera_options.padding(segment, edgeInsets(options.padding(), arena));
    }
    if (options.hasAnchor()) {
      fields |= MapLibreNativeC.MLN_CAMERA_OPTION_ANCHOR();
      mln_camera_options.anchor(segment, screenPoint(options.anchor(), arena));
    }
    if (options.hasZoom()) {
      fields |= MapLibreNativeC.MLN_CAMERA_OPTION_ZOOM();
      mln_camera_options.zoom(segment, options.zoom());
    }
    if (options.hasBearing()) {
      fields |= MapLibreNativeC.MLN_CAMERA_OPTION_BEARING();
      mln_camera_options.bearing(segment, options.bearing());
    }
    if (options.hasPitch()) {
      fields |= MapLibreNativeC.MLN_CAMERA_OPTION_PITCH();
      mln_camera_options.pitch(segment, options.pitch());
    }
    if (options.hasRoll()) {
      fields |= MapLibreNativeC.MLN_CAMERA_OPTION_ROLL();
      mln_camera_options.roll(segment, options.roll());
    }
    if (options.hasFieldOfView()) {
      fields |= MapLibreNativeC.MLN_CAMERA_OPTION_FOV();
      mln_camera_options.field_of_view(segment, options.fieldOfView());
    }
    mln_camera_options.fields(segment, fields);
    return segment;
  }

  public static CameraOptions cameraOptions(MemorySegment segment) {
    var fields = mln_camera_options.fields(segment);
    var options = new CameraOptions();
    if ((fields & MapLibreNativeC.MLN_CAMERA_OPTION_CENTER()) != 0) {
      options.setCenter(
          mln_camera_options.latitude(segment), mln_camera_options.longitude(segment));
    }
    if ((fields & MapLibreNativeC.MLN_CAMERA_OPTION_CENTER_ALTITUDE()) != 0) {
      options.setCenterAltitude(mln_camera_options.center_altitude(segment));
    }
    if ((fields & MapLibreNativeC.MLN_CAMERA_OPTION_PADDING()) != 0) {
      options.setPadding(edgeInsets(mln_camera_options.padding(segment)));
    }
    if ((fields & MapLibreNativeC.MLN_CAMERA_OPTION_ANCHOR()) != 0) {
      options.setAnchor(screenPoint(mln_camera_options.anchor(segment)));
    }
    if ((fields & MapLibreNativeC.MLN_CAMERA_OPTION_ZOOM()) != 0) {
      options.setZoom(mln_camera_options.zoom(segment));
    }
    if ((fields & MapLibreNativeC.MLN_CAMERA_OPTION_BEARING()) != 0) {
      options.setBearing(mln_camera_options.bearing(segment));
    }
    if ((fields & MapLibreNativeC.MLN_CAMERA_OPTION_PITCH()) != 0) {
      options.setPitch(mln_camera_options.pitch(segment));
    }
    if ((fields & MapLibreNativeC.MLN_CAMERA_OPTION_ROLL()) != 0) {
      options.setRoll(mln_camera_options.roll(segment));
    }
    if ((fields & MapLibreNativeC.MLN_CAMERA_OPTION_FOV()) != 0) {
      options.setFieldOfView(mln_camera_options.field_of_view(segment));
    }
    return options;
  }

  public static MemorySegment animationOptions(AnimationOptions options, Arena arena) {
    var segment = MapLibreNativeC.mln_animation_options_default(arena);
    var fields = 0;
    if (options.hasDurationMs()) {
      fields |= MapLibreNativeC.MLN_ANIMATION_OPTION_DURATION();
      mln_animation_options.duration_ms(segment, options.durationMs());
    }
    if (options.hasVelocity()) {
      fields |= MapLibreNativeC.MLN_ANIMATION_OPTION_VELOCITY();
      mln_animation_options.velocity(segment, options.velocity());
    }
    if (options.hasMinZoom()) {
      fields |= MapLibreNativeC.MLN_ANIMATION_OPTION_MIN_ZOOM();
      mln_animation_options.min_zoom(segment, options.minZoom());
    }
    if (options.hasEasing()) {
      fields |= MapLibreNativeC.MLN_ANIMATION_OPTION_EASING();
      mln_animation_options.easing(segment, unitBezier(options.easing(), arena));
    }
    mln_animation_options.fields(segment, fields);
    return segment;
  }

  public static MemorySegment cameraFitOptions(CameraFitOptions options, Arena arena) {
    var segment = MapLibreNativeC.mln_camera_fit_options_default(arena);
    var fields = 0;
    if (options.hasPadding()) {
      fields |= MapLibreNativeC.MLN_CAMERA_FIT_OPTION_PADDING();
      mln_camera_fit_options.padding(segment, edgeInsets(options.padding(), arena));
    }
    if (options.hasBearing()) {
      fields |= MapLibreNativeC.MLN_CAMERA_FIT_OPTION_BEARING();
      mln_camera_fit_options.bearing(segment, options.bearing());
    }
    if (options.hasPitch()) {
      fields |= MapLibreNativeC.MLN_CAMERA_FIT_OPTION_PITCH();
      mln_camera_fit_options.pitch(segment, options.pitch());
    }
    mln_camera_fit_options.fields(segment, fields);
    return segment;
  }

  public static MemorySegment boundOptions(BoundOptions options, Arena arena) {
    var segment = MapLibreNativeC.mln_bound_options_default(arena);
    var fields = 0;
    if (options.hasBounds()) {
      fields |= MapLibreNativeC.MLN_BOUND_OPTION_BOUNDS();
      mln_bound_options.bounds(segment, latLngBounds(options.bounds(), arena));
    }
    if (options.hasMinZoom()) {
      fields |= MapLibreNativeC.MLN_BOUND_OPTION_MIN_ZOOM();
      mln_bound_options.min_zoom(segment, options.minZoom());
    }
    if (options.hasMaxZoom()) {
      fields |= MapLibreNativeC.MLN_BOUND_OPTION_MAX_ZOOM();
      mln_bound_options.max_zoom(segment, options.maxZoom());
    }
    if (options.hasMinPitch()) {
      fields |= MapLibreNativeC.MLN_BOUND_OPTION_MIN_PITCH();
      mln_bound_options.min_pitch(segment, options.minPitch());
    }
    if (options.hasMaxPitch()) {
      fields |= MapLibreNativeC.MLN_BOUND_OPTION_MAX_PITCH();
      mln_bound_options.max_pitch(segment, options.maxPitch());
    }
    mln_bound_options.fields(segment, fields);
    return segment;
  }

  public static BoundOptions boundOptions(MemorySegment segment) {
    var fields = mln_bound_options.fields(segment);
    var options = new BoundOptions();
    if ((fields & MapLibreNativeC.MLN_BOUND_OPTION_BOUNDS()) != 0) {
      options.setBounds(latLngBounds(mln_bound_options.bounds(segment)));
    }
    if ((fields & MapLibreNativeC.MLN_BOUND_OPTION_MIN_ZOOM()) != 0) {
      options.setMinZoom(mln_bound_options.min_zoom(segment));
    }
    if ((fields & MapLibreNativeC.MLN_BOUND_OPTION_MAX_ZOOM()) != 0) {
      options.setMaxZoom(mln_bound_options.max_zoom(segment));
    }
    if ((fields & MapLibreNativeC.MLN_BOUND_OPTION_MIN_PITCH()) != 0) {
      options.setMinPitch(mln_bound_options.min_pitch(segment));
    }
    if ((fields & MapLibreNativeC.MLN_BOUND_OPTION_MAX_PITCH()) != 0) {
      options.setMaxPitch(mln_bound_options.max_pitch(segment));
    }
    return options;
  }

  public static MemorySegment viewportOptions(MapViewportOptions options, Arena arena) {
    var segment = MapLibreNativeC.mln_map_viewport_options_default(arena);
    var fields = 0;
    if (options.hasNorthOrientation()) {
      fields |= MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION();
      mln_map_viewport_options.north_orientation(segment, options.northOrientation().nativeValue());
    }
    if (options.hasConstrainMode()) {
      fields |= MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE();
      mln_map_viewport_options.constrain_mode(segment, options.constrainMode().nativeValue());
    }
    if (options.hasViewportMode()) {
      fields |= MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE();
      mln_map_viewport_options.viewport_mode(segment, options.viewportMode().nativeValue());
    }
    if (options.hasFrustumOffset()) {
      fields |= MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET();
      mln_map_viewport_options.frustum_offset(segment, edgeInsets(options.frustumOffset(), arena));
    }
    mln_map_viewport_options.fields(segment, fields);
    return segment;
  }

  public static MapViewportOptions viewportOptions(MemorySegment segment) {
    var fields = mln_map_viewport_options.fields(segment);
    var options = new MapViewportOptions();
    if ((fields & MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION()) != 0) {
      options.setNorthOrientation(
          NorthOrientation.fromNative(mln_map_viewport_options.north_orientation(segment)));
    }
    if ((fields & MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE()) != 0) {
      options.setConstrainMode(
          ConstrainMode.fromNative(mln_map_viewport_options.constrain_mode(segment)));
    }
    if ((fields & MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE()) != 0) {
      options.setViewportMode(
          ViewportMode.fromNative(mln_map_viewport_options.viewport_mode(segment)));
    }
    if ((fields & MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET()) != 0) {
      options.setFrustumOffset(edgeInsets(mln_map_viewport_options.frustum_offset(segment)));
    }
    return options;
  }

  public static MemorySegment tileOptions(MapTileOptions options, Arena arena) {
    var segment = MapLibreNativeC.mln_map_tile_options_default(arena);
    var fields = 0;
    if (options.hasPrefetchZoomDelta()) {
      fields |= MapLibreNativeC.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA();
      mln_map_tile_options.prefetch_zoom_delta(segment, options.prefetchZoomDelta());
    }
    if (options.hasLodMinRadius()) {
      fields |= MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS();
      mln_map_tile_options.lod_min_radius(segment, options.lodMinRadius());
    }
    if (options.hasLodScale()) {
      fields |= MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_SCALE();
      mln_map_tile_options.lod_scale(segment, options.lodScale());
    }
    if (options.hasLodPitchThreshold()) {
      fields |= MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD();
      mln_map_tile_options.lod_pitch_threshold(segment, options.lodPitchThreshold());
    }
    if (options.hasLodZoomShift()) {
      fields |= MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT();
      mln_map_tile_options.lod_zoom_shift(segment, options.lodZoomShift());
    }
    if (options.hasLodMode()) {
      fields |= MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_MODE();
      mln_map_tile_options.lod_mode(segment, options.lodMode().nativeValue());
    }
    mln_map_tile_options.fields(segment, fields);
    return segment;
  }

  public static MapTileOptions tileOptions(MemorySegment segment) {
    var fields = mln_map_tile_options.fields(segment);
    var options = new MapTileOptions();
    if ((fields & MapLibreNativeC.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA()) != 0) {
      options.setPrefetchZoomDelta(mln_map_tile_options.prefetch_zoom_delta(segment));
    }
    if ((fields & MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS()) != 0) {
      options.setLodMinRadius(mln_map_tile_options.lod_min_radius(segment));
    }
    if ((fields & MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_SCALE()) != 0) {
      options.setLodScale(mln_map_tile_options.lod_scale(segment));
    }
    if ((fields & MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD()) != 0) {
      options.setLodPitchThreshold(mln_map_tile_options.lod_pitch_threshold(segment));
    }
    if ((fields & MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT()) != 0) {
      options.setLodZoomShift(mln_map_tile_options.lod_zoom_shift(segment));
    }
    if ((fields & MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_MODE()) != 0) {
      options.setLodMode(TileLodMode.fromNative(mln_map_tile_options.lod_mode(segment)));
    }
    return options;
  }

  public static MemorySegment projectionModeOptions(ProjectionModeOptions options, Arena arena) {
    var segment = MapLibreNativeC.mln_projection_mode_default(arena);
    var fields = 0;
    if (options.hasAxonometric()) {
      fields |= MapLibreNativeC.MLN_PROJECTION_MODE_AXONOMETRIC();
      mln_projection_mode.axonometric(segment, options.axonometric());
    }
    if (options.hasXSkew()) {
      fields |= MapLibreNativeC.MLN_PROJECTION_MODE_X_SKEW();
      mln_projection_mode.x_skew(segment, options.xSkew());
    }
    if (options.hasYSkew()) {
      fields |= MapLibreNativeC.MLN_PROJECTION_MODE_Y_SKEW();
      mln_projection_mode.y_skew(segment, options.ySkew());
    }
    mln_projection_mode.fields(segment, fields);
    return segment;
  }

  public static ProjectionModeOptions projectionModeOptions(MemorySegment segment) {
    var fields = mln_projection_mode.fields(segment);
    var options = new ProjectionModeOptions();
    if ((fields & MapLibreNativeC.MLN_PROJECTION_MODE_AXONOMETRIC()) != 0) {
      options.setAxonometric(mln_projection_mode.axonometric(segment));
    }
    if ((fields & MapLibreNativeC.MLN_PROJECTION_MODE_X_SKEW()) != 0) {
      options.setXSkew(mln_projection_mode.x_skew(segment));
    }
    if ((fields & MapLibreNativeC.MLN_PROJECTION_MODE_Y_SKEW()) != 0) {
      options.setYSkew(mln_projection_mode.y_skew(segment));
    }
    return options;
  }

  public static MemorySegment freeCameraOptions(FreeCameraOptions options, Arena arena) {
    var segment = MapLibreNativeC.mln_free_camera_options_default(arena);
    var fields = 0;
    if (options.hasPosition()) {
      fields |= MapLibreNativeC.MLN_FREE_CAMERA_OPTION_POSITION();
      mln_free_camera_options.position(segment, vec3(options.position(), arena));
    }
    if (options.hasOrientation()) {
      fields |= MapLibreNativeC.MLN_FREE_CAMERA_OPTION_ORIENTATION();
      mln_free_camera_options.orientation(segment, quaternion(options.orientation(), arena));
    }
    mln_free_camera_options.fields(segment, fields);
    return segment;
  }

  public static FreeCameraOptions freeCameraOptions(MemorySegment segment) {
    var fields = mln_free_camera_options.fields(segment);
    var options = new FreeCameraOptions();
    if ((fields & MapLibreNativeC.MLN_FREE_CAMERA_OPTION_POSITION()) != 0) {
      options.setPosition(vec3(mln_free_camera_options.position(segment)));
    }
    if ((fields & MapLibreNativeC.MLN_FREE_CAMERA_OPTION_ORIENTATION()) != 0) {
      options.setOrientation(quaternion(mln_free_camera_options.orientation(segment)));
    }
    return options;
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

  public static MemorySegment latLngBounds(LatLngBounds bounds, Arena arena) {
    var segment = mln_lat_lng_bounds.allocate(arena);
    mln_lat_lng_bounds.southwest(segment, latLng(bounds.southwest(), arena));
    mln_lat_lng_bounds.northeast(segment, latLng(bounds.northeast(), arena));
    return segment;
  }

  public static LatLngBounds latLngBounds(MemorySegment segment) {
    return new LatLngBounds(
        latLng(mln_lat_lng_bounds.southwest(segment)),
        latLng(mln_lat_lng_bounds.northeast(segment)));
  }

  public static MemorySegment latLngArray(List<LatLng> coordinates, Arena arena) {
    var array = mln_lat_lng.allocateArray(coordinates.size(), arena);
    for (var index = 0; index < coordinates.size(); index++) {
      var coordinate = coordinates.get(index);
      var element = mln_lat_lng.asSlice(array, index);
      mln_lat_lng.latitude(element, coordinate.latitude());
      mln_lat_lng.longitude(element, coordinate.longitude());
    }
    return array;
  }

  public static List<LatLng> latLngArray(MemorySegment segment, int count) {
    var coordinates = new ArrayList<LatLng>(count);
    for (var index = 0; index < count; index++) {
      coordinates.add(latLng(mln_lat_lng.asSlice(segment, index)));
    }
    return List.copyOf(coordinates);
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

  public static MemorySegment screenPointArray(List<ScreenPoint> points, Arena arena) {
    var array = mln_screen_point.allocateArray(points.size(), arena);
    for (var index = 0; index < points.size(); index++) {
      var point = points.get(index);
      var element = mln_screen_point.asSlice(array, index);
      mln_screen_point.x(element, point.x());
      mln_screen_point.y(element, point.y());
    }
    return array;
  }

  public static List<ScreenPoint> screenPointArray(MemorySegment segment, int count) {
    var points = new ArrayList<ScreenPoint>(count);
    for (var index = 0; index < count; index++) {
      points.add(screenPoint(mln_screen_point.asSlice(segment, index)));
    }
    return List.copyOf(points);
  }

  public static MemorySegment unitBezier(UnitBezier easing, Arena arena) {
    var segment = mln_unit_bezier.allocate(arena);
    mln_unit_bezier.x1(segment, easing.x1());
    mln_unit_bezier.y1(segment, easing.y1());
    mln_unit_bezier.x2(segment, easing.x2());
    mln_unit_bezier.y2(segment, easing.y2());
    return segment;
  }

  public static MemorySegment vec3(Vec3 value, Arena arena) {
    var segment = mln_vec3.allocate(arena);
    mln_vec3.x(segment, value.x());
    mln_vec3.y(segment, value.y());
    mln_vec3.z(segment, value.z());
    return segment;
  }

  public static Vec3 vec3(MemorySegment segment) {
    return new Vec3(mln_vec3.x(segment), mln_vec3.y(segment), mln_vec3.z(segment));
  }

  public static MemorySegment quaternion(Quaternion value, Arena arena) {
    var segment = mln_quaternion.allocate(arena);
    mln_quaternion.x(segment, value.x());
    mln_quaternion.y(segment, value.y());
    mln_quaternion.z(segment, value.z());
    mln_quaternion.w(segment, value.w());
    return segment;
  }

  public static Quaternion quaternion(MemorySegment segment) {
    return new Quaternion(
        mln_quaternion.x(segment),
        mln_quaternion.y(segment),
        mln_quaternion.z(segment),
        mln_quaternion.w(segment));
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

  public static EdgeInsets edgeInsets(MemorySegment segment) {
    return new EdgeInsets(
        mln_edge_insets.top(segment),
        mln_edge_insets.left(segment),
        mln_edge_insets.bottom(segment),
        mln_edge_insets.right(segment));
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

  public static MemorySegment jsonValue(JsonValue value, Arena arena) {
    var segment = mln_json_value.allocate(arena);
    writeJsonValue(segment, value, arena, 0);
    return segment;
  }

  public static JsonValue jsonValue(MemorySegment segment) {
    return readJsonValue(segment.reinterpret(mln_json_value.sizeof()), 0);
  }

  public static MemorySegment geometry(Geometry geometry, Arena arena) {
    return geometry(geometry, arena, 0);
  }

  public static Geometry geometry(MemorySegment segment) {
    return readGeometry(segment.reinterpret(mln_geometry.sizeof()), 0);
  }

  public static MemorySegment feature(Feature feature, Arena arena) {
    return feature(feature, arena, 0);
  }

  public static Feature feature(MemorySegment segment) {
    return readFeature(segment.reinterpret(mln_feature.sizeof()), 0);
  }

  public static MemorySegment geoJson(GeoJson geoJson, Arena arena) {
    var segment = mln_geojson.allocate(arena);
    writeGeoJson(segment, geoJson, arena);
    return segment;
  }

  public static GeoJson geoJson(MemorySegment segment) {
    return readGeoJson(segment.reinterpret(mln_geojson.sizeof()));
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

  public static ResourceRequest resourceRequest(MemorySegment segment) {
    var rawKind = mln_resource_request.kind(segment);
    var rawLoadingMethod = mln_resource_request.loading_method(segment);
    var rawPriority = mln_resource_request.priority(segment);
    var rawUsage = mln_resource_request.usage(segment);
    var rawStoragePolicy = mln_resource_request.storage_policy(segment);
    return new ResourceRequest(
        MemoryUtil.copyCString(mln_resource_request.url(segment)),
        ResourceKind.fromNative(rawKind),
        rawKind,
        ResourceLoadingMethod.fromNative(rawLoadingMethod),
        rawLoadingMethod,
        ResourcePriority.fromNative(rawPriority),
        rawPriority,
        ResourceUsage.fromNative(rawUsage),
        rawUsage,
        ResourceStoragePolicy.fromNative(rawStoragePolicy),
        rawStoragePolicy,
        resourceRange(segment),
        optionalLong(
            mln_resource_request.has_prior_modified(segment),
            mln_resource_request.prior_modified_unix_ms(segment)),
        optionalLong(
            mln_resource_request.has_prior_expires(segment),
            mln_resource_request.prior_expires_unix_ms(segment)),
        optionalString(mln_resource_request.prior_etag(segment)),
        MemoryUtil.copyBytes(
            mln_resource_request.prior_data(segment),
            mln_resource_request.prior_data_size(segment)));
  }

  public static MemorySegment resourceResponse(ResourceResponse response, Arena arena) {
    var segment = mln_resource_response.allocate(arena);
    mln_resource_response.size(segment, (int) mln_resource_response.sizeof());
    mln_resource_response.status(segment, response.status().nativeValue());
    mln_resource_response.error_reason(segment, response.errorReason().nativeValue());
    var bytes = response.bytes();
    if (bytes.length > 0) {
      var nativeBytes = arena.allocate(bytes.length);
      MemorySegment.copy(bytes, 0, nativeBytes, ValueLayout.JAVA_BYTE, 0, bytes.length);
      mln_resource_response.bytes(segment, nativeBytes);
      mln_resource_response.byte_count(segment, bytes.length);
    }
    response
        .errorMessage()
        .ifPresent(
            value ->
                mln_resource_response.error_message(
                    segment, MemoryUtil.allocateCString(arena, value)));
    mln_resource_response.must_revalidate(segment, response.mustRevalidate());
    response
        .modifiedUnixMs()
        .ifPresent(
            value -> {
              mln_resource_response.has_modified(segment, true);
              mln_resource_response.modified_unix_ms(segment, value);
            });
    response
        .expiresUnixMs()
        .ifPresent(
            value -> {
              mln_resource_response.has_expires(segment, true);
              mln_resource_response.expires_unix_ms(segment, value);
            });
    response
        .etag()
        .ifPresent(
            value -> mln_resource_response.etag(segment, MemoryUtil.allocateCString(arena, value)));
    response
        .retryAfterUnixMs()
        .ifPresent(
            value -> {
              mln_resource_response.has_retry_after(segment, true);
              mln_resource_response.retry_after_unix_ms(segment, value);
            });
    return segment;
  }

  private static void writeJsonValue(
      MemorySegment segment, JsonValue value, Arena arena, int depth) {
    if (depth > JsonValue.MAX_DESCRIPTOR_DEPTH) {
      throw new IllegalArgumentException("JSON descriptor depth exceeds 64");
    }
    mln_json_value.size(segment, (int) mln_json_value.sizeof());
    var data = mln_json_value.data(segment);
    if (value instanceof JsonValue.Null) {
      mln_json_value.type(segment, MapLibreNativeC.MLN_JSON_VALUE_TYPE_NULL());
    } else if (value instanceof JsonValue.Bool boolValue) {
      mln_json_value.type(segment, MapLibreNativeC.MLN_JSON_VALUE_TYPE_BOOL());
      mln_json_value.data.bool_value(data, boolValue.value());
    } else if (value instanceof JsonValue.UInt uintValue) {
      mln_json_value.type(segment, MapLibreNativeC.MLN_JSON_VALUE_TYPE_UINT());
      mln_json_value.data.uint_value(data, uintValue.value().longValue());
    } else if (value instanceof JsonValue.Int intValue) {
      mln_json_value.type(segment, MapLibreNativeC.MLN_JSON_VALUE_TYPE_INT());
      mln_json_value.data.int_value(data, intValue.value());
    } else if (value instanceof JsonValue.DoubleValue doubleValue) {
      mln_json_value.type(segment, MapLibreNativeC.MLN_JSON_VALUE_TYPE_DOUBLE());
      mln_json_value.data.double_value(data, doubleValue.value());
    } else if (value instanceof JsonValue.StringValue stringValue) {
      mln_json_value.type(segment, MapLibreNativeC.MLN_JSON_VALUE_TYPE_STRING());
      mln_json_value.data.string_value(data, stringView(stringValue.value(), arena));
    } else if (value instanceof JsonValue.Array arrayValue) {
      mln_json_value.type(segment, MapLibreNativeC.MLN_JSON_VALUE_TYPE_ARRAY());
      var values = arrayValue.values();
      var array = mln_json_value.data.array_value(data);
      if (!values.isEmpty()) {
        var nativeValues = mln_json_value.allocateArray(values.size(), arena);
        for (var index = 0; index < values.size(); index++) {
          writeJsonValue(
              mln_json_value.asSlice(nativeValues, index), values.get(index), arena, depth + 1);
        }
        mln_json_array.values(array, nativeValues);
      }
      mln_json_array.value_count(array, values.size());
    } else if (value instanceof JsonValue.ObjectValue objectValue) {
      mln_json_value.type(segment, MapLibreNativeC.MLN_JSON_VALUE_TYPE_OBJECT());
      var members = objectValue.members();
      var object = mln_json_value.data.object_value(data);
      if (!members.isEmpty()) {
        mln_json_object.members(object, jsonMembers(members, arena, depth + 1));
      }
      mln_json_object.member_count(object, members.size());
    } else {
      throw new IllegalArgumentException(
          "Unsupported JSON value type: " + value.getClass().getName());
    }
  }

  private static JsonValue readJsonValue(MemorySegment segment, int depth) {
    if (depth > JsonValue.MAX_DESCRIPTOR_DEPTH) {
      throw new IllegalArgumentException("JSON descriptor depth exceeds 64");
    }
    var data = mln_json_value.data(segment);
    var type = mln_json_value.type(segment);
    if (type == MapLibreNativeC.MLN_JSON_VALUE_TYPE_NULL()) {
      return JsonValue.nullValue();
    }
    if (type == MapLibreNativeC.MLN_JSON_VALUE_TYPE_BOOL()) {
      return JsonValue.of(mln_json_value.data.bool_value(data));
    }
    if (type == MapLibreNativeC.MLN_JSON_VALUE_TYPE_UINT()) {
      return JsonValue.unsigned(
          new BigInteger(Long.toUnsignedString(mln_json_value.data.uint_value(data))));
    }
    if (type == MapLibreNativeC.MLN_JSON_VALUE_TYPE_INT()) {
      return JsonValue.of(mln_json_value.data.int_value(data));
    }
    if (type == MapLibreNativeC.MLN_JSON_VALUE_TYPE_DOUBLE()) {
      return JsonValue.of(mln_json_value.data.double_value(data));
    }
    if (type == MapLibreNativeC.MLN_JSON_VALUE_TYPE_STRING()) {
      return JsonValue.of(stringView(mln_json_value.data.string_value(data)));
    }
    if (type == MapLibreNativeC.MLN_JSON_VALUE_TYPE_ARRAY()) {
      var array = mln_json_value.data.array_value(data);
      var count = Math.toIntExact(mln_json_array.value_count(array));
      var values =
          sizedArray(
              mln_json_array.values(array), count, mln_json_value.sizeof(), "JSON array values");
      var copied = new ArrayList<JsonValue>(count);
      for (var index = 0; index < count; index++) {
        copied.add(readJsonValue(mln_json_value.asSlice(values, index), depth + 1));
      }
      return JsonValue.array(copied);
    }
    if (type == MapLibreNativeC.MLN_JSON_VALUE_TYPE_OBJECT()) {
      var object = mln_json_value.data.object_value(data);
      var count = Math.toIntExact(mln_json_object.member_count(object));
      return JsonValue.object(readJsonMembers(mln_json_object.members(object), count, depth + 1));
    }
    throw new IllegalArgumentException("Unknown JSON value type: " + Integer.toUnsignedLong(type));
  }

  private static MemorySegment jsonMembers(List<JsonValue.Member> members, Arena arena, int depth) {
    var nativeMembers = mln_json_member.allocateArray(members.size(), arena);
    for (var index = 0; index < members.size(); index++) {
      var member = members.get(index);
      var nativeMember = mln_json_member.asSlice(nativeMembers, index);
      mln_json_member.key(nativeMember, stringView(member.key(), arena));
      var nativeValue = mln_json_value.allocate(arena);
      writeJsonValue(nativeValue, member.value(), arena, depth);
      mln_json_member.value(nativeMember, nativeValue);
    }
    return nativeMembers;
  }

  private static List<JsonValue.Member> readJsonMembers(
      MemorySegment members, int count, int depth) {
    var sizedMembers = sizedArray(members, count, mln_json_member.sizeof(), "JSON object members");
    var copied = new ArrayList<JsonValue.Member>(count);
    for (var index = 0; index < count; index++) {
      var member = mln_json_member.asSlice(sizedMembers, index);
      var value =
          sizedPointer(mln_json_member.value(member), mln_json_value.sizeof(), "JSON member value");
      copied.add(
          new JsonValue.Member(
              stringView(mln_json_member.key(member)), readJsonValue(value, depth)));
    }
    return List.copyOf(copied);
  }

  private static void writeGeometry(
      MemorySegment segment, Geometry geometry, Arena arena, int depth) {
    if (depth > Geometry.MAX_COLLECTION_DEPTH) {
      throw new IllegalArgumentException("geometry collection depth exceeds 64");
    }
    mln_geometry.size(segment, (int) mln_geometry.sizeof());
    var data = mln_geometry.data(segment);
    if (geometry instanceof Geometry.Empty) {
      mln_geometry.type(segment, MapLibreNativeC.MLN_GEOMETRY_TYPE_EMPTY());
    } else if (geometry instanceof Geometry.Point point) {
      mln_geometry.type(segment, MapLibreNativeC.MLN_GEOMETRY_TYPE_POINT());
      mln_geometry.data.point(data, latLng(point.coordinate(), arena));
    } else if (geometry instanceof Geometry.LineString lineString) {
      mln_geometry.type(segment, MapLibreNativeC.MLN_GEOMETRY_TYPE_LINE_STRING());
      mln_geometry.data.line_string(data, coordinateSpan(lineString.coordinates(), arena));
    } else if (geometry instanceof Geometry.Polygon polygon) {
      mln_geometry.type(segment, MapLibreNativeC.MLN_GEOMETRY_TYPE_POLYGON());
      mln_geometry.data.polygon(data, polygonGeometry(polygon.rings(), arena));
    } else if (geometry instanceof Geometry.MultiPoint multiPoint) {
      mln_geometry.type(segment, MapLibreNativeC.MLN_GEOMETRY_TYPE_MULTI_POINT());
      mln_geometry.data.multi_point(data, coordinateSpan(multiPoint.coordinates(), arena));
    } else if (geometry instanceof Geometry.MultiLineString multiLineString) {
      mln_geometry.type(segment, MapLibreNativeC.MLN_GEOMETRY_TYPE_MULTI_LINE_STRING());
      mln_geometry.data.multi_line_string(data, multiLineGeometry(multiLineString.lines(), arena));
    } else if (geometry instanceof Geometry.MultiPolygon multiPolygon) {
      mln_geometry.type(segment, MapLibreNativeC.MLN_GEOMETRY_TYPE_MULTI_POLYGON());
      mln_geometry.data.multi_polygon(data, multiPolygonGeometry(multiPolygon.polygons(), arena));
    } else if (geometry instanceof Geometry.Collection collection) {
      mln_geometry.type(segment, MapLibreNativeC.MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION());
      var nativeCollection = mln_geometry.data.geometry_collection(data);
      var geometries = collection.geometries();
      if (!geometries.isEmpty()) {
        var nativeGeometries = mln_geometry.allocateArray(geometries.size(), arena);
        for (var index = 0; index < geometries.size(); index++) {
          writeGeometry(
              mln_geometry.asSlice(nativeGeometries, index),
              geometries.get(index),
              arena,
              depth + 1);
        }
        mln_geometry_collection.geometries(nativeCollection, nativeGeometries);
      }
      mln_geometry_collection.geometry_count(nativeCollection, geometries.size());
    } else {
      throw new IllegalArgumentException(
          "Unsupported geometry type: " + geometry.getClass().getName());
    }
  }

  private static Geometry readGeometry(MemorySegment segment, int depth) {
    if (depth > Geometry.MAX_COLLECTION_DEPTH) {
      throw new IllegalArgumentException("geometry collection depth exceeds 64");
    }
    var data = mln_geometry.data(segment);
    var type = mln_geometry.type(segment);
    if (type == MapLibreNativeC.MLN_GEOMETRY_TYPE_EMPTY()) {
      return Geometry.empty();
    }
    if (type == MapLibreNativeC.MLN_GEOMETRY_TYPE_POINT()) {
      return Geometry.point(latLng(mln_geometry.data.point(data)));
    }
    if (type == MapLibreNativeC.MLN_GEOMETRY_TYPE_LINE_STRING()) {
      return Geometry.lineString(coordinateSpan(mln_geometry.data.line_string(data)));
    }
    if (type == MapLibreNativeC.MLN_GEOMETRY_TYPE_POLYGON()) {
      return Geometry.polygon(polygonGeometry(mln_geometry.data.polygon(data)));
    }
    if (type == MapLibreNativeC.MLN_GEOMETRY_TYPE_MULTI_POINT()) {
      return Geometry.multiPoint(coordinateSpan(mln_geometry.data.multi_point(data)));
    }
    if (type == MapLibreNativeC.MLN_GEOMETRY_TYPE_MULTI_LINE_STRING()) {
      return Geometry.multiLineString(multiLineGeometry(mln_geometry.data.multi_line_string(data)));
    }
    if (type == MapLibreNativeC.MLN_GEOMETRY_TYPE_MULTI_POLYGON()) {
      return Geometry.multiPolygon(multiPolygonGeometry(mln_geometry.data.multi_polygon(data)));
    }
    if (type == MapLibreNativeC.MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION()) {
      var collection = mln_geometry.data.geometry_collection(data);
      var count = Math.toIntExact(mln_geometry_collection.geometry_count(collection));
      var geometries =
          sizedArray(
              mln_geometry_collection.geometries(collection),
              count,
              mln_geometry.sizeof(),
              "geometry collection");
      var copied = new ArrayList<Geometry>(count);
      for (var index = 0; index < count; index++) {
        copied.add(readGeometry(mln_geometry.asSlice(geometries, index), depth + 1));
      }
      return Geometry.collection(copied);
    }
    throw new IllegalArgumentException("Unknown geometry type: " + Integer.toUnsignedLong(type));
  }

  private static MemorySegment coordinateSpan(List<LatLng> coordinates, Arena arena) {
    var span = mln_coordinate_span.allocate(arena);
    if (!coordinates.isEmpty()) {
      mln_coordinate_span.coordinates(span, latLngArray(coordinates, arena));
    }
    mln_coordinate_span.coordinate_count(span, coordinates.size());
    return span;
  }

  private static List<LatLng> coordinateSpan(MemorySegment span) {
    var count = Math.toIntExact(mln_coordinate_span.coordinate_count(span));
    var coordinates =
        sizedArray(
            mln_coordinate_span.coordinates(span), count, mln_lat_lng.sizeof(), "coordinates");
    return latLngArray(coordinates, count);
  }

  private static MemorySegment polygonGeometry(List<List<LatLng>> rings, Arena arena) {
    var polygon = mln_polygon_geometry.allocate(arena);
    if (!rings.isEmpty()) {
      var nativeRings = coordinateSpans(rings, arena);
      mln_polygon_geometry.rings(polygon, nativeRings);
    }
    mln_polygon_geometry.ring_count(polygon, rings.size());
    return polygon;
  }

  private static List<List<LatLng>> polygonGeometry(MemorySegment polygon) {
    var count = Math.toIntExact(mln_polygon_geometry.ring_count(polygon));
    return coordinateSpans(mln_polygon_geometry.rings(polygon), count);
  }

  private static MemorySegment multiLineGeometry(List<List<LatLng>> lines, Arena arena) {
    var multiLine = mln_multi_line_geometry.allocate(arena);
    if (!lines.isEmpty()) {
      mln_multi_line_geometry.lines(multiLine, coordinateSpans(lines, arena));
    }
    mln_multi_line_geometry.line_count(multiLine, lines.size());
    return multiLine;
  }

  private static List<List<LatLng>> multiLineGeometry(MemorySegment multiLine) {
    var count = Math.toIntExact(mln_multi_line_geometry.line_count(multiLine));
    return coordinateSpans(mln_multi_line_geometry.lines(multiLine), count);
  }

  private static MemorySegment multiPolygonGeometry(
      List<List<List<LatLng>>> polygons, Arena arena) {
    var multiPolygon = mln_multi_polygon_geometry.allocate(arena);
    if (!polygons.isEmpty()) {
      var nativePolygons = mln_polygon_geometry.allocateArray(polygons.size(), arena);
      for (var index = 0; index < polygons.size(); index++) {
        mln_polygon_geometry
            .asSlice(nativePolygons, index)
            .copyFrom(polygonGeometry(polygons.get(index), arena));
      }
      mln_multi_polygon_geometry.polygons(multiPolygon, nativePolygons);
    }
    mln_multi_polygon_geometry.polygon_count(multiPolygon, polygons.size());
    return multiPolygon;
  }

  private static List<List<List<LatLng>>> multiPolygonGeometry(MemorySegment multiPolygon) {
    var count = Math.toIntExact(mln_multi_polygon_geometry.polygon_count(multiPolygon));
    var polygons =
        sizedArray(
            mln_multi_polygon_geometry.polygons(multiPolygon),
            count,
            mln_polygon_geometry.sizeof(),
            "multi-polygon polygons");
    var copied = new ArrayList<List<List<LatLng>>>(count);
    for (var index = 0; index < count; index++) {
      copied.add(polygonGeometry(mln_polygon_geometry.asSlice(polygons, index)));
    }
    return List.copyOf(copied);
  }

  private static MemorySegment coordinateSpans(List<List<LatLng>> spans, Arena arena) {
    var nativeSpans = mln_coordinate_span.allocateArray(spans.size(), arena);
    for (var index = 0; index < spans.size(); index++) {
      mln_coordinate_span
          .asSlice(nativeSpans, index)
          .copyFrom(coordinateSpan(spans.get(index), arena));
    }
    return nativeSpans;
  }

  private static List<List<LatLng>> coordinateSpans(MemorySegment spans, int count) {
    var nativeSpans = sizedArray(spans, count, mln_coordinate_span.sizeof(), "coordinate spans");
    var copied = new ArrayList<List<LatLng>>(count);
    for (var index = 0; index < count; index++) {
      copied.add(coordinateSpan(mln_coordinate_span.asSlice(nativeSpans, index)));
    }
    return List.copyOf(copied);
  }

  private static MemorySegment geometry(Geometry geometry, Arena arena, int depth) {
    var segment = mln_geometry.allocate(arena);
    writeGeometry(segment, geometry, arena, depth);
    return segment;
  }

  private static MemorySegment feature(Feature feature, Arena arena, int depth) {
    var segment = mln_feature.allocate(arena);
    writeFeature(segment, feature, arena, depth);
    return segment;
  }

  private static void writeFeature(MemorySegment segment, Feature feature, Arena arena, int depth) {
    mln_feature.size(segment, (int) mln_feature.sizeof());
    mln_feature.geometry(segment, geometry(feature.geometry(), arena, depth + 1));
    var properties = feature.properties();
    if (!properties.isEmpty()) {
      mln_feature.properties(segment, jsonMembers(properties, arena, depth + 1));
    }
    mln_feature.property_count(segment, properties.size());
    writeFeatureIdentifier(segment, feature.identifier(), arena);
  }

  private static Feature readFeature(MemorySegment segment, int depth) {
    var geometry =
        readGeometry(
            sizedPointer(mln_feature.geometry(segment), mln_geometry.sizeof(), "feature geometry"),
            depth + 1);
    var propertyCount = Math.toIntExact(mln_feature.property_count(segment));
    var properties = readJsonMembers(mln_feature.properties(segment), propertyCount, depth + 1);
    return new Feature(geometry, properties, readFeatureIdentifier(segment));
  }

  private static void writeFeatureIdentifier(
      MemorySegment segment, FeatureIdentifier identifier, Arena arena) {
    var data = mln_feature.identifier(segment);
    if (identifier instanceof FeatureIdentifier.Null) {
      mln_feature.identifier_type(segment, MapLibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_NULL());
    } else if (identifier instanceof FeatureIdentifier.UInt value) {
      mln_feature.identifier_type(segment, MapLibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_UINT());
      mln_feature.identifier.uint_value(data, value.value().longValue());
    } else if (identifier instanceof FeatureIdentifier.Int value) {
      mln_feature.identifier_type(segment, MapLibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_INT());
      mln_feature.identifier.int_value(data, value.value());
    } else if (identifier instanceof FeatureIdentifier.DoubleValue value) {
      mln_feature.identifier_type(segment, MapLibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE());
      mln_feature.identifier.double_value(data, value.value());
    } else if (identifier instanceof FeatureIdentifier.StringValue value) {
      mln_feature.identifier_type(segment, MapLibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_STRING());
      mln_feature.identifier.string_value(data, stringView(value.value(), arena));
    } else {
      throw new IllegalArgumentException(
          "Unsupported feature identifier type: " + identifier.getClass().getName());
    }
  }

  private static FeatureIdentifier readFeatureIdentifier(MemorySegment segment) {
    var data = mln_feature.identifier(segment);
    var type = mln_feature.identifier_type(segment);
    if (type == MapLibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_NULL()) {
      return FeatureIdentifier.nullValue();
    }
    if (type == MapLibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_UINT()) {
      return FeatureIdentifier.unsigned(
          new BigInteger(Long.toUnsignedString(mln_feature.identifier.uint_value(data))));
    }
    if (type == MapLibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_INT()) {
      return FeatureIdentifier.of(mln_feature.identifier.int_value(data));
    }
    if (type == MapLibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE()) {
      return FeatureIdentifier.of(mln_feature.identifier.double_value(data));
    }
    if (type == MapLibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_STRING()) {
      return FeatureIdentifier.of(stringView(mln_feature.identifier.string_value(data)));
    }
    throw new IllegalArgumentException(
        "Unknown feature identifier type: " + Integer.toUnsignedLong(type));
  }

  private static void writeGeoJson(MemorySegment segment, GeoJson geoJson, Arena arena) {
    mln_geojson.size(segment, (int) mln_geojson.sizeof());
    var data = mln_geojson.data(segment);
    if (geoJson instanceof GeoJson.GeometryValue geometryValue) {
      mln_geojson.type(segment, MapLibreNativeC.MLN_GEOJSON_TYPE_GEOMETRY());
      mln_geojson.data.geometry(data, geometry(geometryValue.geometry(), arena));
    } else if (geoJson instanceof GeoJson.FeatureValue featureValue) {
      mln_geojson.type(segment, MapLibreNativeC.MLN_GEOJSON_TYPE_FEATURE());
      mln_geojson.data.feature(data, feature(featureValue.feature(), arena, 0));
    } else if (geoJson instanceof GeoJson.FeatureCollection featureCollection) {
      mln_geojson.type(segment, MapLibreNativeC.MLN_GEOJSON_TYPE_FEATURE_COLLECTION());
      var features = featureCollection.features();
      var nativeCollection = mln_geojson.data.feature_collection(data);
      if (!features.isEmpty()) {
        var nativeFeatures = mln_feature.allocateArray(features.size(), arena);
        for (var index = 0; index < features.size(); index++) {
          writeFeature(mln_feature.asSlice(nativeFeatures, index), features.get(index), arena, 1);
        }
        mln_feature_collection.features(nativeCollection, nativeFeatures);
      }
      mln_feature_collection.feature_count(nativeCollection, features.size());
    } else {
      throw new IllegalArgumentException(
          "Unsupported GeoJSON type: " + geoJson.getClass().getName());
    }
  }

  private static GeoJson readGeoJson(MemorySegment segment) {
    var data = mln_geojson.data(segment);
    var type = mln_geojson.type(segment);
    if (type == MapLibreNativeC.MLN_GEOJSON_TYPE_GEOMETRY()) {
      return GeoJson.geometry(
          readGeometry(
              sizedPointer(
                  mln_geojson.data.geometry(data), mln_geometry.sizeof(), "GeoJSON geometry"),
              0));
    }
    if (type == MapLibreNativeC.MLN_GEOJSON_TYPE_FEATURE()) {
      return GeoJson.feature(
          readFeature(
              sizedPointer(mln_geojson.data.feature(data), mln_feature.sizeof(), "GeoJSON feature"),
              0));
    }
    if (type == MapLibreNativeC.MLN_GEOJSON_TYPE_FEATURE_COLLECTION()) {
      var collection = mln_geojson.data.feature_collection(data);
      var count = Math.toIntExact(mln_feature_collection.feature_count(collection));
      var features =
          sizedArray(
              mln_feature_collection.features(collection),
              count,
              mln_feature.sizeof(),
              "GeoJSON feature collection");
      var copied = new ArrayList<Feature>(count);
      for (var index = 0; index < count; index++) {
        copied.add(readFeature(mln_feature.asSlice(features, index), 1));
      }
      return GeoJson.featureCollection(copied);
    }
    throw new IllegalArgumentException("Unknown GeoJSON type: " + Integer.toUnsignedLong(type));
  }

  private static MemorySegment stringView(String value, Arena arena) {
    var bytes = value.getBytes(StandardCharsets.UTF_8);
    var view = mln_string_view.allocate(arena);
    if (bytes.length > 0) {
      var nativeBytes = arena.allocate(bytes.length);
      MemorySegment.copy(bytes, 0, nativeBytes, ValueLayout.JAVA_BYTE, 0, bytes.length);
      mln_string_view.data(view, nativeBytes);
    }
    mln_string_view.size(view, bytes.length);
    return view;
  }

  private static String stringView(MemorySegment view) {
    return MemoryUtil.copyStringView(mln_string_view.data(view), mln_string_view.size(view));
  }

  private static MemorySegment sizedPointer(MemorySegment pointer, long byteSize, String name) {
    if (MemoryUtil.isNull(pointer)) {
      throw new IllegalArgumentException(name + " must not be null");
    }
    return pointer.reinterpret(byteSize);
  }

  private static MemorySegment sizedArray(
      MemorySegment pointer, int count, long elementSize, String name) {
    if (count == 0) {
      return MemorySegment.NULL;
    }
    if (MemoryUtil.isNull(pointer)) {
      throw new IllegalArgumentException(name + " must not be null when count is non-zero");
    }
    return pointer.reinterpret(elementSize * count);
  }

  private static Optional<ResourceRequest.ByteRange> resourceRange(MemorySegment segment) {
    if (!mln_resource_request.has_range(segment)) {
      return Optional.empty();
    }
    return Optional.of(
        new ResourceRequest.ByteRange(
            mln_resource_request.range_start(segment), mln_resource_request.range_end(segment)));
  }

  private static Optional<Long> optionalLong(boolean present, long value) {
    return present ? Optional.of(value) : Optional.empty();
  }

  private static Optional<String> optionalString(MemorySegment value) {
    return MemoryUtil.isNull(value) ? Optional.empty() : Optional.of(MemoryUtil.copyCString(value));
  }
}
