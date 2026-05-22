package org.maplibre.nativejni.internal.bridge;

/** JNI declarations for the BaseNative C API coverage group. */
public final class BaseNative {
  private BaseNative() {}

  public static native long mln_c_version();

  public static native int mln_supported_render_backend_mask();

  public static native String mln_thread_last_error_message();
}
