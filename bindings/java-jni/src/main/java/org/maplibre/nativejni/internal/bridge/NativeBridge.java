package org.maplibre.nativejni.internal.bridge;

/** Small Java facade for internal callers that do not need direct JavaCPP access. */
public final class NativeBridge {
  private NativeBridge() {}

  public static long cVersion() {
    return BaseNative.mln_c_version();
  }

  public static int supportedRenderBackendMask() {
    return BaseNative.mln_supported_render_backend_mask();
  }

  public static int networkStatusGet(int[] outStatus) {
    return RuntimeNative.mln_network_status_get(outStatus);
  }

  public static int networkStatusSet(int status) {
    return RuntimeNative.mln_network_status_set(status);
  }

  public static String threadLastErrorMessage() {
    return BaseNative.mln_thread_last_error_message();
  }
}
