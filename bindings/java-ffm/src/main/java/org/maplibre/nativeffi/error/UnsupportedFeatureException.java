package org.maplibre.nativeffi.error;

/** Thrown when the native library does not support the requested feature. */
public final class UnsupportedFeatureException extends MaplibreException {
  public UnsupportedFeatureException(int nativeStatusCode, String diagnostic) {
    super(MaplibreStatus.UNSUPPORTED, nativeStatusCode, diagnostic);
  }
}
