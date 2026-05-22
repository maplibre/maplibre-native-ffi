package org.maplibre.nativejni.internal.bridge;

/** JNI declarations for the RuntimeNative C API coverage group. */
public final class RuntimeNative {
  private RuntimeNative() {}

  public static native int mln_network_status_get(int[] outStatus);

  public static native int mln_network_status_set(int status);

  public static native int mln_runtime_options_default();

  public static native int mln_runtime_create(long[] outRuntime);

  public static native int mln_runtime_set_resource_provider();

  public static native int mln_resource_request_complete();

  public static native int mln_resource_request_cancelled();

  public static native int mln_resource_request_release();

  public static native int mln_runtime_set_resource_transform();

  public static native int mln_runtime_clear_resource_transform();

  public static native int mln_runtime_run_ambient_cache_operation_start();

  public static native int mln_runtime_offline_operation_discard();

  public static native int mln_runtime_destroy(long runtime);

  public static native int mln_runtime_run_once(long runtime);

  public static native int mln_runtime_poll_event();
}
