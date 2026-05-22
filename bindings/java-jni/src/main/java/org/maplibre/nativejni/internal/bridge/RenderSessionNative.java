package org.maplibre.nativejni.internal.bridge;

/** JNI declarations for the RenderSessionNative C API coverage group. */
public final class RenderSessionNative {
  private RenderSessionNative() {}

  public static native int mln_render_session_resize(
      long session, int width, int height, double scaleFactor);

  public static native int mln_render_session_render_update(long session);

  public static native int mln_render_session_detach(long session);

  public static native int mln_render_session_destroy(long session);

  public static native int mln_render_session_reduce_memory_use(long session);

  public static native int mln_render_session_clear_data(long session);

  public static native int mln_render_session_dump_debug_logs(long session);

  public static native int mln_render_session_set_feature_state();

  public static native int mln_render_session_get_feature_state();

  public static native int mln_render_session_remove_feature_state();

  public static native int mln_json_snapshot_get();

  public static native int mln_json_snapshot_destroy();
}
