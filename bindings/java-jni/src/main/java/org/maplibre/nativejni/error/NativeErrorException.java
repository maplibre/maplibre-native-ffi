package org.maplibre.nativejni.error;

/** Thrown when a native Maplibre error or C++ exception crosses the C ABI as a status. */
public final class NativeErrorException extends MaplibreException {
  public NativeErrorException(int nativeStatusCode, String diagnostic) {
    super(MaplibreStatus.NATIVE_ERROR, nativeStatusCode, diagnostic);
  }
}
