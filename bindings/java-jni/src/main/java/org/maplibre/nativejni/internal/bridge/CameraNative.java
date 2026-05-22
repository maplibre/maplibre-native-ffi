package org.maplibre.nativejni.internal.bridge;

/** JNI declarations for the CameraNative C API coverage group. */
public final class CameraNative {
  private CameraNative() {}

  public static native int mln_camera_options_default();

  public static native int mln_animation_options_default();

  public static native int mln_camera_fit_options_default();

  public static native int mln_bound_options_default();

  public static native int mln_free_camera_options_default();

  public static native int mln_projection_mode_default();

  public static native int mln_map_viewport_options_default();

  public static native int mln_map_tile_options_default();

  public static native int mln_map_set_debug_options(long map, int options);

  public static native int mln_map_get_debug_options(long map, int[] outOptions);

  public static native int mln_map_set_rendering_stats_view_enabled(long map, boolean enabled);

  public static native int mln_map_get_rendering_stats_view_enabled(long map, boolean[] outEnabled);

  public static native int mln_map_is_fully_loaded(long map, boolean[] outLoaded);

  public static native int mln_map_dump_debug_logs(long map);

  public static native int mln_map_get_viewport_options();

  public static native int mln_map_set_viewport_options();

  public static native int mln_map_get_tile_options();

  public static native int mln_map_set_tile_options();

  public static native int mln_map_get_camera(long map, boolean[] outFields, double[] outValues);

  public static native int mln_map_jump_to(long map, boolean[] cameraFields, double[] cameraValues);

  public static native int mln_map_ease_to(
      long map,
      boolean[] cameraFields,
      double[] cameraValues,
      boolean hasAnimation,
      boolean[] animationFields,
      double[] animationValues);

  public static native int mln_map_fly_to(
      long map,
      boolean[] cameraFields,
      double[] cameraValues,
      boolean hasAnimation,
      boolean[] animationFields,
      double[] animationValues);

  public static native int mln_map_move_by(long map, double deltaX, double deltaY);

  public static native int mln_map_move_by_animated(
      long map,
      double deltaX,
      double deltaY,
      boolean hasAnimation,
      boolean[] animationFields,
      double[] animationValues);

  public static native int mln_map_scale_by(
      long map, double scale, boolean hasAnchor, double anchorX, double anchorY);

  public static native int mln_map_scale_by_animated(
      long map,
      double scale,
      boolean hasAnchor,
      double anchorX,
      double anchorY,
      boolean hasAnimation,
      boolean[] animationFields,
      double[] animationValues);

  public static native int mln_map_rotate_by(
      long map, double firstX, double firstY, double secondX, double secondY);

  public static native int mln_map_rotate_by_animated(
      long map,
      double firstX,
      double firstY,
      double secondX,
      double secondY,
      boolean hasAnimation,
      boolean[] animationFields,
      double[] animationValues);

  public static native int mln_map_pitch_by(long map, double pitch);

  public static native int mln_map_pitch_by_animated(
      long map,
      double pitch,
      boolean hasAnimation,
      boolean[] animationFields,
      double[] animationValues);

  public static native int mln_map_cancel_transitions(long map);

  public static native int mln_map_camera_for_lat_lng_bounds(
      long map,
      double southwestLatitude,
      double southwestLongitude,
      double northeastLatitude,
      double northeastLongitude,
      boolean hasFitOptions,
      boolean[] fitFields,
      double[] fitValues,
      boolean[] outCameraFields,
      double[] outCameraValues);

  public static native int mln_map_camera_for_lat_lngs(
      long map,
      double[] coordinates,
      boolean hasFitOptions,
      boolean[] fitFields,
      double[] fitValues,
      boolean[] outCameraFields,
      double[] outCameraValues);

  public static native int mln_map_camera_for_geometry();

  public static native int mln_map_lat_lng_bounds_for_camera(
      long map, boolean[] cameraFields, double[] cameraValues, double[] outBounds);

  public static native int mln_map_lat_lng_bounds_for_camera_unwrapped(
      long map, boolean[] cameraFields, double[] cameraValues, double[] outBounds);

  public static native int mln_map_get_bounds(long map, boolean[] outFields, double[] outValues);

  public static native int mln_map_set_bounds(long map, boolean[] fields, double[] values);

  public static native int mln_map_get_free_camera_options(
      long map, boolean[] outFields, double[] outValues);

  public static native int mln_map_set_free_camera_options(
      long map, boolean[] fields, double[] values);

  public static native int mln_map_get_projection_mode(
      long map, boolean[] outFields, boolean[] outBooleans, double[] outValues);

  public static native int mln_map_set_projection_mode(
      long map, boolean[] fields, boolean[] booleans, double[] values);

  public static native int mln_map_pixel_for_lat_lng(
      long map, double latitude, double longitude, double[] outPoint);

  public static native int mln_map_lat_lng_for_pixel(
      long map, double x, double y, double[] outCoordinate);

  public static native int mln_map_pixels_for_lat_lngs(
      long map, double[] coordinates, double[] outPoints);

  public static native int mln_map_lat_lngs_for_pixels(
      long map, double[] points, double[] outCoordinates);
}
