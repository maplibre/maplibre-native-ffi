package org.maplibre.nativeffi.internal;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import org.maplibre.nativeffi.AnimationOptions;
import org.maplibre.nativeffi.BoundOptions;
import org.maplibre.nativeffi.CameraFitOptions;
import org.maplibre.nativeffi.CameraOptions;
import org.maplibre.nativeffi.ConstrainMode;
import org.maplibre.nativeffi.EdgeInsets;
import org.maplibre.nativeffi.FreeCameraOptions;
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
import org.maplibre.nativeffi.internal.c.mln_edge_insets;
import org.maplibre.nativeffi.internal.c.mln_free_camera_options;
import org.maplibre.nativeffi.internal.c.mln_lat_lng;
import org.maplibre.nativeffi.internal.c.mln_lat_lng_bounds;
import org.maplibre.nativeffi.internal.c.mln_map_options;
import org.maplibre.nativeffi.internal.c.mln_map_tile_options;
import org.maplibre.nativeffi.internal.c.mln_map_viewport_options;
import org.maplibre.nativeffi.internal.c.mln_offline_region_status;
import org.maplibre.nativeffi.internal.c.mln_projected_meters;
import org.maplibre.nativeffi.internal.c.mln_projection_mode;
import org.maplibre.nativeffi.internal.c.mln_quaternion;
import org.maplibre.nativeffi.internal.c.mln_rendering_stats;
import org.maplibre.nativeffi.internal.c.mln_resource_request;
import org.maplibre.nativeffi.internal.c.mln_resource_response;
import org.maplibre.nativeffi.internal.c.mln_runtime_options;
import org.maplibre.nativeffi.internal.c.mln_screen_point;
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
