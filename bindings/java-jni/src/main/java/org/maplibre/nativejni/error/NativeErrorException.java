package org.maplibre.nativejni.error;

/** Thrown when native MapLibre code or C++ exception conversion reports failure. */
public final class NativeErrorException extends MaplibreException {
  public NativeErrorException(int nativeStatusCode, String diagnostic) {
    super(MaplibreStatus.NATIVE_ERROR, nativeStatusCode, diagnostic);
  }
}
