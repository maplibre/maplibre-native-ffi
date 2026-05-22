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

  public static native int mln_map_get_camera();

  public static native int mln_map_jump_to();

  public static native int mln_map_ease_to();

  public static native int mln_map_fly_to();

  public static native int mln_map_move_by(long map, double deltaX, double deltaY);

  public static native int mln_map_move_by_animated(long map, double deltaX, double deltaY);

  public static native int mln_map_scale_by(
      long map, double scale, boolean hasAnchor, double anchorX, double anchorY);

  public static native int mln_map_scale_by_animated(
      long map, double scale, boolean hasAnchor, double anchorX, double anchorY);

  public static native int mln_map_rotate_by(
      long map, double firstX, double firstY, double secondX, double secondY);

  public static native int mln_map_rotate_by_animated(
      long map, double firstX, double firstY, double secondX, double secondY);

  public static native int mln_map_pitch_by(long map, double pitch);

  public static native int mln_map_pitch_by_animated(long map, double pitch);

  public static native int mln_map_cancel_transitions(long map);

  public static native int mln_map_camera_for_lat_lng_bounds();

  public static native int mln_map_camera_for_lat_lngs();

  public static native int mln_map_camera_for_geometry();

  public static native int mln_map_lat_lng_bounds_for_camera();

  public static native int mln_map_lat_lng_bounds_for_camera_unwrapped();

  public static native int mln_map_get_bounds();

  public static native int mln_map_set_bounds();

  public static native int mln_map_get_free_camera_options();

  public static native int mln_map_set_free_camera_options();

  public static native int mln_map_get_projection_mode();

  public static native int mln_map_set_projection_mode();

  public static native int mln_map_pixel_for_lat_lng();

  public static native int mln_map_lat_lng_for_pixel();

  public static native int mln_map_pixels_for_lat_lngs();

  public static native int mln_map_lat_lngs_for_pixels();
}
