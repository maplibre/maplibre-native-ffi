package org.maplibre.nativejni.internal.bridge;

/** JNI declarations for the LogNative C API coverage group. */
public final class LogNative {
  private LogNative() {}

  public static native int mln_log_set_callback();

  public static native int mln_log_clear_callback();

  public static native int mln_log_set_async_severity_mask();
}
