package org.maplibre.nativeffi;

/** Thrown when a native MapLibre error or C++ exception crosses the C ABI as a status. */
public final class NativeErrorException extends MapLibreException {
  public NativeErrorException(int nativeStatusCode, String diagnostic) {
    super(MapLibreStatus.NATIVE_ERROR, nativeStatusCode, diagnostic);
  }
}
