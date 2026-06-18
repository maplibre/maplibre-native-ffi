package org.maplibre.nativeffi.internal.status;

import org.maplibre.nativeffi.error.InvalidStateException;
import org.maplibre.nativeffi.error.MaplibreStatus;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.convert.NativeValues;
import org.maplibre.nativeffi.internal.memory.MemoryUtil;

/** Converts native status codes into public Java exceptions. */
public final class Status {
  private Status() {}

  public static void check(int nativeStatus) {
    var status = NativeValues.maplibreStatus(nativeStatus);
    if (status == MaplibreStatus.OK) {
      return;
    }
    throw NativeValues.exceptionForStatus(status, nativeStatus, captureDiagnostic());
  }

  public static InvalidStateException released(String typeName) {
    return new InvalidStateException(
        NativeValues.nativeCode(MaplibreStatus.INVALID_STATE), typeName + " is already closed");
  }

  public static InvalidStateException callbackReentry(String typeName) {
    return new InvalidStateException(
        NativeValues.nativeCode(MaplibreStatus.INVALID_STATE),
        typeName + " cannot be closed from its callback");
  }

  public static String captureDiagnostic() {
    return MemoryUtil.copyCString(MapLibreNativeC.mln_thread_last_error_message());
  }
}
