package org.maplibre.nativejni.internal.bridge;

/** JNI declarations for the ProjectionNative C API coverage group. */
public final class ProjectionNative {
  private ProjectionNative() {}

  public static native int mln_map_projection_create(long map, long[] outProjection);

  public static native int mln_map_projection_destroy(long projection);

  public static native int mln_map_projection_get_camera();

  public static native int mln_map_projection_set_camera();

  public static native int mln_map_projection_set_visible_coordinates();

  public static native int mln_map_projection_set_visible_geometry();

  public static native int mln_map_projection_pixel_for_lat_lng();

  public static native int mln_map_projection_lat_lng_for_pixel();

  public static native int mln_projected_meters_for_lat_lng();

  public static native int mln_lat_lng_for_projected_meters();
}
