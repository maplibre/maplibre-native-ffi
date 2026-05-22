package org.maplibre.nativejni.internal.bridge;

/** JNI declarations for the RuntimeNative C API coverage group. */
public final class RuntimeNative {
  private RuntimeNative() {}

  public static native int mln_network_status_get(int[] outStatus);

  public static native int mln_network_status_set(int status);

  public static native int mln_runtime_options_default();

  public static native int mln_runtime_create(long[] outRuntime);

  public static native int mln_runtime_set_resource_provider(
      long runtime,
      org.maplibre.nativejni.resource.ResourceProviderCallback callback,
      long[] outState);

  public static native int mln_resource_request_complete(
      long handle,
      org.maplibre.nativejni.internal.struct.ResourceStructs.ResourceResponseValue response);

  public static native int mln_resource_request_cancelled(long handle, boolean[] outCancelled);

  public static native void mln_resource_request_release(long handle);

  public static native void mln_resource_provider_state_destroy(long state);

  public static native int mln_runtime_set_resource_transform(
      long runtime,
      org.maplibre.nativejni.resource.ResourceTransformCallback callback,
      long[] outState);

  public static native int mln_runtime_clear_resource_transform(long runtime);

  public static native void mln_resource_transform_state_destroy(long state);

  public static native int mln_runtime_run_ambient_cache_operation_start(
      long runtime, int operation, long[] outOperationId);

  public static native int mln_runtime_offline_operation_discard(long runtime, long operationId);

  public static native int mln_runtime_destroy(long runtime);

  public static native int mln_runtime_run_once(long runtime);

  public static native int mln_runtime_poll_event(
      long runtime,
      long[] longs,
      int[] ints,
      boolean[] booleans,
      double[] doubles,
      String[] strings);
}
