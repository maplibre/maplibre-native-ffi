package org.maplibre.nativejni.internal.bridge;

/** JNI declarations for the RenderSessionNative C API coverage group. */
public final class RenderSessionNative {
  private RenderSessionNative() {}

  public static native int mln_render_session_resize();

  public static native int mln_render_session_render_update();

  public static native int mln_render_session_detach();

  public static native int mln_render_session_destroy();

  public static native int mln_render_session_reduce_memory_use();

  public static native int mln_render_session_clear_data();

  public static native int mln_render_session_dump_debug_logs();

  public static native int mln_render_session_set_feature_state();

  public static native int mln_render_session_get_feature_state();

  public static native int mln_render_session_remove_feature_state();

  public static native int mln_json_snapshot_get();

  public static native int mln_json_snapshot_destroy();
}
