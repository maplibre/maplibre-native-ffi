package org.maplibre.nativejni.internal.status;

import org.maplibre.nativejni.error.InvalidStateException;
import org.maplibre.nativejni.error.MaplibreException;
import org.maplibre.nativejni.error.MaplibreStatus;
import org.maplibre.nativejni.internal.bridge.NativeBridge;

/** Converts native status codes into public Java exceptions. */
public final class Status {
  private Status() {}

  public static void check(int nativeStatus) {
    var status = MaplibreStatus.fromNative(nativeStatus);
    if (status == MaplibreStatus.OK) {
      return;
    }
    throw MaplibreException.forStatus(status, nativeStatus, captureDiagnostic());
  }

  public static InvalidStateException released(String typeName) {
    return new InvalidStateException(
        MaplibreStatus.INVALID_STATE.nativeCode(), typeName + " is already closed");
  }

  public static String captureDiagnostic() {
    return NativeBridge.threadLastErrorMessage();
  }
}
