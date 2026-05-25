package org.maplibre.nativejni.internal.bridge;

/** Legacy JNI bridge declarations backed by generated native exports and registration metadata. */
public final class NativeBridge {
  private NativeBridge() {}

  public static native long cVersion();

  public static native int supportedRenderBackendMask();

  public static native int networkStatusGet(int[] outStatus);

  public static native int networkStatusSet(int status);

  public static native String threadLastErrorMessage();
}
