package org.maplibre.nativejni.internal.bridge;

/** JNI method declarations registered by the Rust bridge from {@code JNI_OnLoad}. */
public final class NativeBridge {
  private NativeBridge() {}

  public static native long cVersion();

  public static native int supportedRenderBackendMask();

  public static native int networkStatusGet(int[] outStatus);

  public static native int networkStatusSet(int status);

  public static native String threadLastErrorMessage();
}
