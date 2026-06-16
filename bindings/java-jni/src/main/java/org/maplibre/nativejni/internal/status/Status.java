package org.maplibre.nativejni.internal.status;

import org.maplibre.nativejni.error.InvalidArgumentException;
import org.maplibre.nativejni.error.InvalidStateException;
import org.maplibre.nativejni.error.MaplibreException;
import org.maplibre.nativejni.error.MaplibreStatus;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;

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

  public static void checkNoEmbeddedNul(String value, String description) {
    if (value.indexOf('\0') >= 0) {
      throw new InvalidArgumentException(
          MaplibreStatus.INVALID_ARGUMENT.nativeCode(), description + " contains embedded NUL");
    }
  }

  public static InvalidStateException releasing(String typeName) {
    return new InvalidStateException(
        MaplibreStatus.INVALID_STATE.nativeCode(), typeName + " is closing");
  }

  public static InvalidStateException callbackReentry(String typeName) {
    return new InvalidStateException(
        MaplibreStatus.INVALID_STATE.nativeCode(),
        typeName + " cannot be closed from its callback");
  }

  public static InvalidStateException liveChildren(String typeName, int childCount) {
    return new InvalidStateException(
        MaplibreStatus.INVALID_STATE.nativeCode(),
        typeName + " has " + childCount + " live child handle(s)");
  }

  public static String captureDiagnostic() {
    return JavaCppSupport.takeThreadDiagnostic()
        .orElseGet(() -> JavaCppSupport.cString(MaplibreNativeC.mln_thread_last_error_message()));
  }
}
