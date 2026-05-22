package org.maplibre.nativejni.internal.callback;

import java.util.Objects;
import org.maplibre.nativejni.internal.bridge.LogNative;
import org.maplibre.nativejni.internal.loader.NativeLibrary;
import org.maplibre.nativejni.internal.status.Status;
import org.maplibre.nativejni.log.LogCallback;

/** Owns process-global logging callback state. */
public final class LogCallbackState {
  private LogCallbackState() {}

  public static void set(LogCallback callback) {
    NativeLibrary.ensureLoaded();
    Status.check(LogNative.mln_log_set_callback(Objects.requireNonNull(callback, "callback")));
  }

  public static void clear() {
    NativeLibrary.ensureLoaded();
    Status.check(LogNative.mln_log_clear_callback());
  }
}
