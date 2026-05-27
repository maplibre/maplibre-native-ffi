package org.maplibre.nativejni.internal.callback;

import java.util.Objects;
import org.bytedeco.javacpp.BytePointer;
import org.bytedeco.javacpp.Pointer;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.internal.loader.NativeLibrary;
import org.maplibre.nativejni.internal.status.Status;
import org.maplibre.nativejni.log.LogCallback;
import org.maplibre.nativejni.log.LogEvent;
import org.maplibre.nativejni.log.LogRecord;
import org.maplibre.nativejni.log.LogSeverity;

/** Owns process-global logging callback state. */
public final class LogCallbackState {
  private static MaplibreNativeC.mln_log_callback currentCallback;

  private LogCallbackState() {}

  public static synchronized void set(LogCallback callback) {
    NativeLibrary.ensureLoaded();
    Objects.requireNonNull(callback, "callback");
    var nativeCallback =
        new MaplibreNativeC.mln_log_callback() {
          @Override
          public int call(
              Pointer userData, int severity, int event, long code, BytePointer message) {
            try {
              return callback.log(
                      new LogRecord(
                          LogSeverity.fromNative(severity),
                          severity,
                          LogEvent.fromNative(event),
                          event,
                          code,
                          JavaCppSupport.cString(message)))
                  ? 1
                  : 0;
            } catch (Throwable exception) {
              return 0;
            }
          }
        };
    try {
      Status.check(MaplibreNativeC.mln_log_set_callback(nativeCallback, null));
    } catch (RuntimeException | Error error) {
      closeQuietly(nativeCallback);
      throw error;
    }
    closeQuietly(currentCallback);
    currentCallback = nativeCallback;
  }

  public static synchronized void clear() {
    NativeLibrary.ensureLoaded();
    Status.check(MaplibreNativeC.mln_log_clear_callback());
    closeQuietly(currentCallback);
    currentCallback = null;
  }

  private static void closeQuietly(MaplibreNativeC.mln_log_callback callback) {
    if (callback == null) {
      return;
    }
    try {
      callback.close();
    } catch (Exception ignored) {
      // Closing callback stubs is best-effort during replacement or teardown.
    }
  }
}
