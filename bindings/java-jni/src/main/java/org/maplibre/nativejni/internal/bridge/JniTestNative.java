package org.maplibre.nativejni.internal.bridge;

/** JNI-only test hooks for boundary behavior that public APIs cannot observe directly. */
public final class JniTestNative {
  private JniTestNative() {}

  public static native int panicStatus();

  public static native int createManyLocalStrings(int count);

  public static native boolean invokeOnAttachedNativeThread(Runnable callback);

  public static native int unregisteredNativeForTesting();
}
