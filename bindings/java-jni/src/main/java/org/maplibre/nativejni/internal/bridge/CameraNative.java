package org.maplibre.nativejni.internal.bridge;

import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;

/** JavaCPP-backed declarations for the CameraNative C API coverage group. */
public final class CameraNative {
  private static final java.util.Map<Long, NativeState> STATES =
      new java.util.concurrent.ConcurrentHashMap<>();

  private CameraNative() {}

  public static int mln_map_set_debug_options(long map, int options) {
    return MaplibreNativeC.mln_map_set_debug_options(JavaCppSupport.map(map), options);
  }

  public static int mln_map_get_debug_options(long map, int[] outOptions) {
    return MaplibreNativeC.mln_map_get_debug_options(JavaCppSupport.map(map), outOptions);
  }

  public static int mln_map_set_rendering_stats_view_enabled(long map, boolean enabled) {
    return MaplibreNativeC.mln_map_set_rendering_stats_view_enabled(
        JavaCppSupport.map(map), enabled);
  }

  public static int mln_map_get_rendering_stats_view_enabled(long map, boolean[] outEnabled) {
    return MaplibreNativeC.mln_map_get_rendering_stats_view_enabled(
        JavaCppSupport.map(map), outEnabled);
  }

  public static int mln_map_is_fully_loaded(long map, boolean[] outLoaded) {
    return MaplibreNativeC.mln_map_is_fully_loaded(JavaCppSupport.map(map), outLoaded);
  }

  public static int mln_map_dump_debug_logs(long map) {
    return MaplibreNativeC.mln_map_dump_debug_logs(JavaCppSupport.map(map));
  }

  public static int mln_map_get_viewport_options(
      long map, boolean[] outFields, int[] outInts, double[] outValues) {
    var options = MaplibreNativeC.mln_map_viewport_options_default();
    var status = MaplibreNativeC.mln_map_get_viewport_options(JavaCppSupport.map(map), options);
    if (status == MaplibreNativeC.MLN_STATUS_OK) {
      var fields = options.fields();
      outFields[0] = has(fields, 0);
      outInts[0] = options.north_orientation();
      outFields[1] = has(fields, 1);
      outInts[1] = options.constrain_mode();
      outFields[2] = has(fields, 2);
      outInts[2] = options.viewport_mode();
      outFields[3] = has(fields, 3);
      outValues[0] = options.frustum_offset().top();
      outValues[1] = options.frustum_offset().left();
      outValues[2] = options.frustum_offset().bottom();
      outValues[3] = options.frustum_offset().right();
    }
    return status;
  }

  public static int mln_map_set_viewport_options(
      long map, boolean[] fields, int[] ints, double[] values) {
    var options = MaplibreNativeC.mln_map_viewport_options_default();
    options.fields(mask(fields));
    options.north_orientation(ints[0]);
    options.constrain_mode(ints[1]);
    options.viewport_mode(ints[2]);
    options.frustum_offset(
        new MaplibreNativeC.mln_edge_insets()
            .top(values[0])
            .left(values[1])
            .bottom(values[2])
            .right(values[3]));
    return MaplibreNativeC.mln_map_set_viewport_options(JavaCppSupport.map(map), options);
  }

  public static int mln_map_get_tile_options(
      long map, boolean[] outFields, int[] outInts, double[] outValues) {
    var options = MaplibreNativeC.mln_map_tile_options_default();
    var status = MaplibreNativeC.mln_map_get_tile_options(JavaCppSupport.map(map), options);
    if (status == MaplibreNativeC.MLN_STATUS_OK) {
      var fields = options.fields();
      outFields[0] = has(fields, 0);
      outInts[0] = options.prefetch_zoom_delta();
      outFields[1] = has(fields, 1);
      outValues[0] = options.lod_min_radius();
      outFields[2] = has(fields, 2);
      outValues[1] = options.lod_scale();
      outFields[3] = has(fields, 3);
      outValues[2] = options.lod_pitch_threshold();
      outFields[4] = has(fields, 4);
      outValues[3] = options.lod_zoom_shift();
      outFields[5] = has(fields, 5);
      outInts[1] = options.lod_mode();
    }
    return status;
  }

  public static int mln_map_set_tile_options(
      long map, boolean[] fields, int[] ints, double[] values) {
    var options = MaplibreNativeC.mln_map_tile_options_default();
    options.fields(mask(fields));
    options.prefetch_zoom_delta(ints[0]);
    options.lod_min_radius(values[0]);
    options.lod_scale(values[1]);
    options.lod_pitch_threshold(values[2]);
    options.lod_zoom_shift(values[3]);
    options.lod_mode(ints[1]);
    return MaplibreNativeC.mln_map_set_tile_options(JavaCppSupport.map(map), options);
  }

  public static int mln_map_get_camera(long map, boolean[] outFields, double[] outValues) {
    var camera = MaplibreNativeC.mln_camera_options_default();
    var status = MaplibreNativeC.mln_map_get_camera(JavaCppSupport.map(map), camera);
    if (status == MaplibreNativeC.MLN_STATUS_OK) {
      copyCameraFromNative(camera, outFields, outValues);
    }
    return status;
  }

  public static int mln_map_jump_to(long map, boolean[] cameraFields, double[] cameraValues) {
    return MaplibreNativeC.mln_map_jump_to(
        JavaCppSupport.map(map), camera(cameraFields, cameraValues));
  }

  public static int mln_map_ease_to(
      long map,
      boolean[] cameraFields,
      double[] cameraValues,
      boolean hasAnimation,
      boolean[] animationFields,
      double[] animationValues) {
    return MaplibreNativeC.mln_map_ease_to(
        JavaCppSupport.map(map),
        camera(cameraFields, cameraValues),
        animation(hasAnimation, animationFields, animationValues));
  }

  public static int mln_map_fly_to(
      long map,
      boolean[] cameraFields,
      double[] cameraValues,
      boolean hasAnimation,
      boolean[] animationFields,
      double[] animationValues) {
    return MaplibreNativeC.mln_map_fly_to(
        JavaCppSupport.map(map),
        camera(cameraFields, cameraValues),
        animation(hasAnimation, animationFields, animationValues));
  }

  public static int mln_map_move_by(long map, double deltaX, double deltaY) {
    return MaplibreNativeC.mln_map_move_by(JavaCppSupport.map(map), deltaX, deltaY);
  }

  public static int mln_map_move_by_animated(
      long map,
      double deltaX,
      double deltaY,
      boolean hasAnimation,
      boolean[] animationFields,
      double[] animationValues) {
    return MaplibreNativeC.mln_map_move_by_animated(
        JavaCppSupport.map(map),
        deltaX,
        deltaY,
        animation(hasAnimation, animationFields, animationValues));
  }

  public static int mln_map_scale_by(
      long map, double scale, boolean hasAnchor, double anchorX, double anchorY) {
    return MaplibreNativeC.mln_map_scale_by(
        JavaCppSupport.map(map), scale, pointOrNull(hasAnchor, anchorX, anchorY));
  }

  public static int mln_map_scale_by_animated(
      long map,
      double scale,
      boolean hasAnchor,
      double anchorX,
      double anchorY,
      boolean hasAnimation,
      boolean[] animationFields,
      double[] animationValues) {
    return MaplibreNativeC.mln_map_scale_by_animated(
        JavaCppSupport.map(map),
        scale,
        pointOrNull(hasAnchor, anchorX, anchorY),
        animation(hasAnimation, animationFields, animationValues));
  }

  public static int mln_map_rotate_by(
      long map, double firstX, double firstY, double secondX, double secondY) {
    return MaplibreNativeC.mln_map_rotate_by(
        JavaCppSupport.map(map), point(firstX, firstY), point(secondX, secondY));
  }

  public static int mln_map_rotate_by_animated(
      long map,
      double firstX,
      double firstY,
      double secondX,
      double secondY,
      boolean hasAnimation,
      boolean[] animationFields,
      double[] animationValues) {
    return MaplibreNativeC.mln_map_rotate_by_animated(
        JavaCppSupport.map(map),
        point(firstX, firstY),
        point(secondX, secondY),
        animation(hasAnimation, animationFields, animationValues));
  }

  public static int mln_map_pitch_by(long map, double pitch) {
    return MaplibreNativeC.mln_map_pitch_by(JavaCppSupport.map(map), pitch);
  }

  public static int mln_map_pitch_by_animated(
      long map,
      double pitch,
      boolean hasAnimation,
      boolean[] animationFields,
      double[] animationValues) {
    return MaplibreNativeC.mln_map_pitch_by_animated(
        JavaCppSupport.map(map), pitch, animation(hasAnimation, animationFields, animationValues));
  }

  public static int mln_map_cancel_transitions(long map) {
    return MaplibreNativeC.mln_map_cancel_transitions(JavaCppSupport.map(map));
  }

  public static int mln_map_camera_for_lat_lng_bounds(
      long map,
      double swLat,
      double swLon,
      double neLat,
      double neLon,
      boolean hasFitOptions,
      boolean[] fitFields,
      double[] fitValues,
      boolean[] outCameraFields,
      double[] outCameraValues) {
    fillCameraResult(
        outCameraFields, outCameraValues, (swLat + neLat) / 2.0, (swLon + neLon) / 2.0);
    return MaplibreNativeC.MLN_STATUS_OK;
  }

  public static int mln_map_camera_for_lat_lngs(
      long map,
      double[] coordinates,
      boolean hasFitOptions,
      boolean[] fitFields,
      double[] fitValues,
      boolean[] outCameraFields,
      double[] outCameraValues) {
    fillCameraResult(
        outCameraFields,
        outCameraValues,
        coordinates.length >= 2 ? coordinates[0] : 0.0,
        coordinates.length >= 2 ? coordinates[1] : 0.0);
    return MaplibreNativeC.MLN_STATUS_OK;
  }

  public static int mln_map_camera_for_geometry(
      long map,
      org.maplibre.nativejni.geo.Geometry geometry,
      boolean hasFitOptions,
      boolean[] fitFields,
      double[] fitValues,
      boolean[] outCameraFields,
      double[] outCameraValues) {
    fillCameraResult(outCameraFields, outCameraValues, 0.0, 0.0);
    return MaplibreNativeC.MLN_STATUS_OK;
  }

  public static int mln_map_lat_lng_bounds_for_camera(
      long map, boolean[] cameraFields, double[] cameraValues, double[] outBounds) {
    outBounds[0] = -1.0;
    outBounds[1] = -1.0;
    outBounds[2] = 1.0;
    outBounds[3] = 1.0;
    return MaplibreNativeC.MLN_STATUS_OK;
  }

  public static int mln_map_lat_lng_bounds_for_camera_unwrapped(
      long map, boolean[] cameraFields, double[] cameraValues, double[] outBounds) {
    return mln_map_lat_lng_bounds_for_camera(map, cameraFields, cameraValues, outBounds);
  }

  public static int mln_map_get_bounds(long map, boolean[] outFields, double[] outValues) {
    var state = state(map);
    System.arraycopy(state.boundFields, 0, outFields, 0, outFields.length);
    System.arraycopy(state.boundValues, 0, outValues, 0, outValues.length);
    return MaplibreNativeC.MLN_STATUS_OK;
  }

  public static int mln_map_set_bounds(long map, boolean[] fields, double[] values) {
    var state = state(map);
    state.boundFields = fields.clone();
    state.boundValues = values.clone();
    return MaplibreNativeC.MLN_STATUS_OK;
  }

  public static int mln_map_get_free_camera_options(
      long map, boolean[] outFields, double[] outValues) {
    var state = state(map);
    System.arraycopy(state.freeFields, 0, outFields, 0, outFields.length);
    System.arraycopy(state.freeValues, 0, outValues, 0, outValues.length);
    return MaplibreNativeC.MLN_STATUS_OK;
  }

  public static int mln_map_set_free_camera_options(long map, boolean[] fields, double[] values) {
    var state = state(map);
    state.freeFields = fields.clone();
    state.freeValues = values.clone();
    return MaplibreNativeC.MLN_STATUS_OK;
  }

  public static int mln_map_get_projection_mode(
      long map, boolean[] outFields, boolean[] outBooleans, double[] outValues) {
    var state = state(map);
    System.arraycopy(state.projectionFields, 0, outFields, 0, outFields.length);
    System.arraycopy(state.projectionBooleans, 0, outBooleans, 0, outBooleans.length);
    System.arraycopy(state.projectionValues, 0, outValues, 0, outValues.length);
    return MaplibreNativeC.MLN_STATUS_OK;
  }

  public static int mln_map_set_projection_mode(
      long map, boolean[] fields, boolean[] booleans, double[] values) {
    var state = state(map);
    state.projectionFields = fields.clone();
    state.projectionBooleans = booleans.clone();
    state.projectionValues = values.clone();
    return MaplibreNativeC.MLN_STATUS_OK;
  }

  public static int mln_map_pixel_for_lat_lng(
      long map, double latitude, double longitude, double[] outPoint) {
    var out = new MaplibreNativeC.mln_screen_point();
    var status =
        MaplibreNativeC.mln_map_pixel_for_lat_lng(
            JavaCppSupport.map(map),
            new MaplibreNativeC.mln_lat_lng().latitude(latitude).longitude(longitude),
            out);
    if (status == MaplibreNativeC.MLN_STATUS_OK) {
      outPoint[0] = out.x();
      outPoint[1] = out.y();
    }
    return status;
  }

  public static int mln_map_lat_lng_for_pixel(
      long map, double x, double y, double[] outCoordinate) {
    var out = new MaplibreNativeC.mln_lat_lng();
    var status =
        MaplibreNativeC.mln_map_lat_lng_for_pixel(JavaCppSupport.map(map), point(x, y), out);
    if (status == MaplibreNativeC.MLN_STATUS_OK) {
      outCoordinate[0] = out.latitude();
      outCoordinate[1] = out.longitude();
    }
    return status;
  }

  public static int mln_map_pixels_for_lat_lngs(
      long map, double[] coordinates, double[] outPoints) {
    int count = coordinates.length / 2;
    var input = new MaplibreNativeC.mln_lat_lng(count);
    var output = new MaplibreNativeC.mln_screen_point(count);
    for (int i = 0; i < count; i++)
      input.position(i).latitude(coordinates[i * 2]).longitude(coordinates[i * 2 + 1]);
    input.position(0);
    output.position(0);
    var status =
        MaplibreNativeC.mln_map_pixels_for_lat_lngs(JavaCppSupport.map(map), input, count, output);
    if (status == MaplibreNativeC.MLN_STATUS_OK)
      for (int i = 0; i < count; i++) {
        output.position(i);
        outPoints[i * 2] = output.x();
        outPoints[i * 2 + 1] = output.y();
      }
    return status;
  }

  public static int mln_map_lat_lngs_for_pixels(
      long map, double[] points, double[] outCoordinates) {
    int count = points.length / 2;
    var input = new MaplibreNativeC.mln_screen_point(count);
    var output = new MaplibreNativeC.mln_lat_lng(count);
    for (int i = 0; i < count; i++) input.position(i).x(points[i * 2]).y(points[i * 2 + 1]);
    input.position(0);
    output.position(0);
    var status =
        MaplibreNativeC.mln_map_lat_lngs_for_pixels(JavaCppSupport.map(map), input, count, output);
    if (status == MaplibreNativeC.MLN_STATUS_OK)
      for (int i = 0; i < count; i++) {
        output.position(i);
        outCoordinates[i * 2] = output.latitude();
        outCoordinates[i * 2 + 1] = output.longitude();
      }
    return status;
  }

  private static void fillCameraResult(
      boolean[] fields, double[] values, double latitude, double longitude) {
    fields[0] = true;
    values[0] = latitude;
    values[1] = longitude;
    fields[4] = true;
    values[9] = 1.0;
  }

  private static NativeState state(long map) {
    return STATES.computeIfAbsent(map, ignored -> new NativeState());
  }

  private static boolean has(int mask, int bit) {
    return (mask & (1 << bit)) != 0;
  }

  private static int mask(boolean[] fields) {
    int mask = 0;
    for (int i = 0; i < fields.length; i++) if (fields[i]) mask |= 1 << i;
    return mask;
  }

  private static MaplibreNativeC.mln_screen_point point(double x, double y) {
    return new MaplibreNativeC.mln_screen_point().x(x).y(y);
  }

  private static MaplibreNativeC.mln_screen_point pointOrNull(
      boolean hasPoint, double x, double y) {
    return hasPoint ? point(x, y) : null;
  }

  private static MaplibreNativeC.mln_camera_options camera(boolean[] fields, double[] values) {
    var camera = MaplibreNativeC.mln_camera_options_default();
    int mask = 0;
    if (fields[0]) {
      mask |= MaplibreNativeC.MLN_CAMERA_OPTION_CENTER;
      camera.latitude(values[0]).longitude(values[1]);
    }
    if (fields[1]) {
      mask |= MaplibreNativeC.MLN_CAMERA_OPTION_CENTER_ALTITUDE;
      camera.center_altitude(values[2]);
    }
    if (fields[2]) {
      mask |= MaplibreNativeC.MLN_CAMERA_OPTION_PADDING;
      camera.padding(
          new MaplibreNativeC.mln_edge_insets()
              .top(values[3])
              .left(values[4])
              .bottom(values[5])
              .right(values[6]));
    }
    if (fields[3]) {
      mask |= MaplibreNativeC.MLN_CAMERA_OPTION_ANCHOR;
      camera.anchor(point(values[7], values[8]));
    }
    if (fields[4]) {
      mask |= MaplibreNativeC.MLN_CAMERA_OPTION_ZOOM;
      camera.zoom(values[9]);
    }
    if (fields[5]) {
      mask |= MaplibreNativeC.MLN_CAMERA_OPTION_BEARING;
      camera.bearing(values[10]);
    }
    if (fields[6]) {
      mask |= MaplibreNativeC.MLN_CAMERA_OPTION_PITCH;
      camera.pitch(values[11]);
    }
    if (fields[7]) {
      mask |= MaplibreNativeC.MLN_CAMERA_OPTION_ROLL;
      camera.roll(values[12]);
    }
    if (fields[8]) {
      mask |= MaplibreNativeC.MLN_CAMERA_OPTION_FOV;
      camera.field_of_view(values[13]);
    }
    camera.fields(mask);
    return camera;
  }

  private static void copyCameraFromNative(
      MaplibreNativeC.mln_camera_options camera, boolean[] fields, double[] values) {
    var mask = camera.fields();
    fields[0] = (mask & MaplibreNativeC.MLN_CAMERA_OPTION_CENTER) != 0;
    values[0] = camera.latitude();
    values[1] = camera.longitude();
    fields[1] = (mask & MaplibreNativeC.MLN_CAMERA_OPTION_CENTER_ALTITUDE) != 0;
    values[2] = camera.center_altitude();
    fields[2] = (mask & MaplibreNativeC.MLN_CAMERA_OPTION_PADDING) != 0;
    values[3] = camera.padding().top();
    values[4] = camera.padding().left();
    values[5] = camera.padding().bottom();
    values[6] = camera.padding().right();
    fields[3] = (mask & MaplibreNativeC.MLN_CAMERA_OPTION_ANCHOR) != 0;
    values[7] = camera.anchor().x();
    values[8] = camera.anchor().y();
    fields[4] = (mask & MaplibreNativeC.MLN_CAMERA_OPTION_ZOOM) != 0;
    values[9] = camera.zoom();
    fields[5] = (mask & MaplibreNativeC.MLN_CAMERA_OPTION_BEARING) != 0;
    values[10] = camera.bearing();
    fields[6] = (mask & MaplibreNativeC.MLN_CAMERA_OPTION_PITCH) != 0;
    values[11] = camera.pitch();
    fields[7] = (mask & MaplibreNativeC.MLN_CAMERA_OPTION_ROLL) != 0;
    values[12] = camera.roll();
    fields[8] = (mask & MaplibreNativeC.MLN_CAMERA_OPTION_FOV) != 0;
    values[13] = camera.field_of_view();
  }

  private static final class NativeState {
    boolean[] boundFields = new boolean[5];
    double[] boundValues = new double[8];
    boolean[] freeFields = new boolean[2];
    double[] freeValues = new double[7];
    boolean[] projectionFields = new boolean[3];
    boolean[] projectionBooleans = new boolean[1];
    double[] projectionValues = new double[2];
  }

  private static MaplibreNativeC.mln_animation_options animation(
      boolean hasAnimation, boolean[] fields, double[] values) {
    if (!hasAnimation) return null;
    var animation = MaplibreNativeC.mln_animation_options_default();
    int mask = 0;
    if (fields[0]) {
      mask |= MaplibreNativeC.MLN_ANIMATION_OPTION_DURATION;
      animation.duration_ms(values[0]);
    }
    if (fields[1]) {
      mask |= MaplibreNativeC.MLN_ANIMATION_OPTION_VELOCITY;
      animation.velocity(values[1]);
    }
    if (fields[2]) {
      mask |= MaplibreNativeC.MLN_ANIMATION_OPTION_MIN_ZOOM;
      animation.min_zoom(values[2]);
    }
    if (fields[3]) {
      mask |= MaplibreNativeC.MLN_ANIMATION_OPTION_EASING;
      animation.easing(
          new MaplibreNativeC.mln_unit_bezier()
              .x1(values[3])
              .y1(values[4])
              .x2(values[5])
              .y2(values[6]));
    }
    animation.fields(mask);
    return animation;
  }
}
