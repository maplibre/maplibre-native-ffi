package org.maplibre.nativeffi.internal;

import org.maplibre.nativeffi.InvalidStateException;
import org.maplibre.nativeffi.MapLibreException;
import org.maplibre.nativeffi.MapLibreStatus;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;

/** Converts native status codes into public Java exceptions. */
public final class Status {
  private Status() {}

  public static void check(int nativeStatus) {
    var status = MapLibreStatus.fromNative(nativeStatus);
    if (status == MapLibreStatus.OK) {
      return;
    }
    throw MapLibreException.forStatus(status, nativeStatus, captureDiagnostic());
  }

  public static InvalidStateException released(String typeName) {
    return new InvalidStateException(
        MapLibreStatus.INVALID_STATE.nativeCode(), typeName + " is already closed");
  }

  public static String captureDiagnostic() {
    return MemoryUtil.copyCString(MapLibreNativeC.mln_thread_last_error_message());
  }
}
