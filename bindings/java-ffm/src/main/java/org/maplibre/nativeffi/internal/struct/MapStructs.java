package org.maplibre.nativeffi.internal.struct;

import static org.maplibre.nativeffi.internal.struct.CoreStructs.edgeInsets;
import static org.maplibre.nativeffi.internal.struct.CoreStructs.latLngBounds;
import static org.maplibre.nativeffi.internal.struct.CoreStructs.quaternion;
import static org.maplibre.nativeffi.internal.struct.CoreStructs.screenPoint;
import static org.maplibre.nativeffi.internal.struct.CoreStructs.unitBezier;
import static org.maplibre.nativeffi.internal.struct.CoreStructs.vec3;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import org.maplibre.nativeffi.camera.AnimationOptions;
import org.maplibre.nativeffi.camera.BoundOptions;
import org.maplibre.nativeffi.camera.CameraFitOptions;
import org.maplibre.nativeffi.camera.CameraOptions;
import org.maplibre.nativeffi.camera.FreeCameraOptions;
import org.maplibre.nativeffi.error.InvalidArgumentException;
import org.maplibre.nativeffi.geo.LatLng;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_animation_options;
import org.maplibre.nativeffi.internal.c.mln_bound_options;
import org.maplibre.nativeffi.internal.c.mln_camera_fit_options;
import org.maplibre.nativeffi.internal.c.mln_camera_options;
import org.maplibre.nativeffi.internal.c.mln_free_camera_options;
import org.maplibre.nativeffi.internal.c.mln_map_options;
import org.maplibre.nativeffi.internal.c.mln_map_tile_options;
import org.maplibre.nativeffi.internal.c.mln_map_viewport_options;
import org.maplibre.nativeffi.internal.c.mln_projection_mode;
import org.maplibre.nativeffi.internal.convert.NativeValues;
import org.maplibre.nativeffi.map.MapOptions;
import org.maplibre.nativeffi.map.ProjectionModeOptions;
import org.maplibre.nativeffi.map.TileOptions;
import org.maplibre.nativeffi.map.ViewportOptions;

/** Internal materializers and readers for map descriptors. */
public final class MapStructs {
  private MapStructs() {}

  public static MemorySegment mapOptions(MapOptions options, Arena arena) {
    var segment = MapLibreNativeC.mln_map_options_default(arena);
    if ((options.width() != null && options.width() < 0)
        || (options.height() != null && options.height() < 0)) {
      throw new InvalidArgumentException(
          MapLibreNativeC.MLN_STATUS_INVALID_ARGUMENT(), "width and height must be non-negative");
    }
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
      mln_map_options.map_mode(segment, NativeValues.nativeValue(options.mapMode()));
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
      options.center(
          new LatLng(mln_camera_options.latitude(segment), mln_camera_options.longitude(segment)));
    }
    if ((fields & MapLibreNativeC.MLN_CAMERA_OPTION_CENTER_ALTITUDE()) != 0) {
      options.centerAltitude(mln_camera_options.center_altitude(segment));
    }
    if ((fields & MapLibreNativeC.MLN_CAMERA_OPTION_PADDING()) != 0) {
      options.padding(edgeInsets(mln_camera_options.padding(segment)));
    }
    if ((fields & MapLibreNativeC.MLN_CAMERA_OPTION_ANCHOR()) != 0) {
      options.anchor(screenPoint(mln_camera_options.anchor(segment)));
    }
    if ((fields & MapLibreNativeC.MLN_CAMERA_OPTION_ZOOM()) != 0) {
      options.zoom(mln_camera_options.zoom(segment));
    }
    if ((fields & MapLibreNativeC.MLN_CAMERA_OPTION_BEARING()) != 0) {
      options.bearing(mln_camera_options.bearing(segment));
    }
    if ((fields & MapLibreNativeC.MLN_CAMERA_OPTION_PITCH()) != 0) {
      options.pitch(mln_camera_options.pitch(segment));
    }
    if ((fields & MapLibreNativeC.MLN_CAMERA_OPTION_ROLL()) != 0) {
      options.roll(mln_camera_options.roll(segment));
    }
    if ((fields & MapLibreNativeC.MLN_CAMERA_OPTION_FOV()) != 0) {
      options.fieldOfView(mln_camera_options.field_of_view(segment));
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
      options.bounds(latLngBounds(mln_bound_options.bounds(segment)));
    }
    if ((fields & MapLibreNativeC.MLN_BOUND_OPTION_MIN_ZOOM()) != 0) {
      options.minZoom(mln_bound_options.min_zoom(segment));
    }
    if ((fields & MapLibreNativeC.MLN_BOUND_OPTION_MAX_ZOOM()) != 0) {
      options.maxZoom(mln_bound_options.max_zoom(segment));
    }
    if ((fields & MapLibreNativeC.MLN_BOUND_OPTION_MIN_PITCH()) != 0) {
      options.minPitch(mln_bound_options.min_pitch(segment));
    }
    if ((fields & MapLibreNativeC.MLN_BOUND_OPTION_MAX_PITCH()) != 0) {
      options.maxPitch(mln_bound_options.max_pitch(segment));
    }
    return options;
  }

  public static MemorySegment viewportOptions(ViewportOptions options, Arena arena) {
    var segment = MapLibreNativeC.mln_map_viewport_options_default(arena);
    var fields = 0;
    if (options.hasNorthOrientation()) {
      fields |= MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION();
      mln_map_viewport_options.north_orientation(
          segment, NativeValues.nativeValue(options.northOrientation()));
    }
    if (options.hasConstrainMode()) {
      fields |= MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE();
      mln_map_viewport_options.constrain_mode(
          segment, NativeValues.nativeValue(options.constrainMode()));
    }
    if (options.hasViewportMode()) {
      fields |= MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE();
      mln_map_viewport_options.viewport_mode(
          segment, NativeValues.nativeValue(options.viewportMode()));
    }
    if (options.hasFrustumOffset()) {
      fields |= MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET();
      mln_map_viewport_options.frustum_offset(segment, edgeInsets(options.frustumOffset(), arena));
    }
    mln_map_viewport_options.fields(segment, fields);
    return segment;
  }

  public static ViewportOptions viewportOptions(MemorySegment segment) {
    var fields = mln_map_viewport_options.fields(segment);
    var options = new ViewportOptions();
    if ((fields & MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION()) != 0) {
      options.northOrientation(
          NativeValues.northOrientation(mln_map_viewport_options.north_orientation(segment)));
    }
    if ((fields & MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE()) != 0) {
      options.constrainMode(
          NativeValues.constrainMode(mln_map_viewport_options.constrain_mode(segment)));
    }
    if ((fields & MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE()) != 0) {
      options.viewportMode(
          NativeValues.viewportMode(mln_map_viewport_options.viewport_mode(segment)));
    }
    if ((fields & MapLibreNativeC.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET()) != 0) {
      options.frustumOffset(edgeInsets(mln_map_viewport_options.frustum_offset(segment)));
    }
    return options;
  }

  public static MemorySegment tileOptions(TileOptions options, Arena arena) {
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
      mln_map_tile_options.lod_mode(segment, NativeValues.nativeValue(options.lodMode()));
    }
    mln_map_tile_options.fields(segment, fields);
    return segment;
  }

  public static TileOptions tileOptions(MemorySegment segment) {
    var fields = mln_map_tile_options.fields(segment);
    var options = new TileOptions();
    if ((fields & MapLibreNativeC.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA()) != 0) {
      options.prefetchZoomDelta(mln_map_tile_options.prefetch_zoom_delta(segment));
    }
    if ((fields & MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS()) != 0) {
      options.lodMinRadius(mln_map_tile_options.lod_min_radius(segment));
    }
    if ((fields & MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_SCALE()) != 0) {
      options.lodScale(mln_map_tile_options.lod_scale(segment));
    }
    if ((fields & MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD()) != 0) {
      options.lodPitchThreshold(mln_map_tile_options.lod_pitch_threshold(segment));
    }
    if ((fields & MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT()) != 0) {
      options.lodZoomShift(mln_map_tile_options.lod_zoom_shift(segment));
    }
    if ((fields & MapLibreNativeC.MLN_MAP_TILE_OPTION_LOD_MODE()) != 0) {
      options.lodMode(NativeValues.tileLodMode(mln_map_tile_options.lod_mode(segment)));
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
      options.axonometric(mln_projection_mode.axonometric(segment));
    }
    if ((fields & MapLibreNativeC.MLN_PROJECTION_MODE_X_SKEW()) != 0) {
      options.xSkew(mln_projection_mode.x_skew(segment));
    }
    if ((fields & MapLibreNativeC.MLN_PROJECTION_MODE_Y_SKEW()) != 0) {
      options.ySkew(mln_projection_mode.y_skew(segment));
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
      options.position(vec3(mln_free_camera_options.position(segment)));
    }
    if ((fields & MapLibreNativeC.MLN_FREE_CAMERA_OPTION_ORIENTATION()) != 0) {
      options.orientation(quaternion(mln_free_camera_options.orientation(segment)));
    }
    return options;
  }
}
