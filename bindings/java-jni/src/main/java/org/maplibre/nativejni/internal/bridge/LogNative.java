package org.maplibre.nativejni.internal.bridge;

import org.bytedeco.javacpp.BytePointer;
import org.bytedeco.javacpp.Pointer;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.log.LogCallback;
import org.maplibre.nativejni.log.LogEvent;
import org.maplibre.nativejni.log.LogRecord;
import org.maplibre.nativejni.log.LogSeverity;

/** JavaCPP-backed declarations for the LogNative C API coverage group. */
public final class LogNative {
  private static MaplibreNativeC.mln_log_callback currentCallback;

  private LogNative() {}

  public static synchronized int mln_log_set_callback(LogCallback callback) {
    if (callback == null) {
      currentCallback = null;
      return MaplibreNativeC.mln_log_set_callback(null, null);
    }
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
    var status = MaplibreNativeC.mln_log_set_callback(nativeCallback, null);
    if (status == MaplibreNativeC.MLN_STATUS_OK) {
      currentCallback = nativeCallback;
    }
    return status;
  }

  public static synchronized int mln_log_clear_callback() {
    var status = MaplibreNativeC.mln_log_clear_callback();
    if (status == MaplibreNativeC.MLN_STATUS_OK) {
      currentCallback = null;
    }
    return status;
  }

  public static int mln_log_set_async_severity_mask(int mask) {
    return MaplibreNativeC.mln_log_set_async_severity_mask(mask);
  }
}
