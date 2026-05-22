package org.maplibre.nativejni.internal.bridge;

/** JNI method declarations registered by the Rust bridge from {@code JNI_OnLoad}. */
public final class NativeBridge {
  private NativeBridge() {}

  public static native long cVersion();
}
