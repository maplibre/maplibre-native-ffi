package org.maplibre.nativejni.internal.bridge;

/** JNI declarations for the MapNative C API coverage group. */
public final class MapNative {
  private MapNative() {}

  public static native int mln_map_options_default();

  public static native int mln_map_create();

  public static native int mln_map_request_repaint();

  public static native int mln_map_request_still_image();

  public static native int mln_map_destroy();

  public static native int mln_map_set_style_url();

  public static native int mln_map_set_style_json();
}
