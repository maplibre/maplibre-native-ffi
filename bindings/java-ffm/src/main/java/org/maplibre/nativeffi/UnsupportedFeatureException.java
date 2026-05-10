package org.maplibre.nativeffi;

/** Thrown when the native library does not support the requested feature. */
public final class UnsupportedFeatureException extends MapLibreException {
  public UnsupportedFeatureException(int nativeStatusCode, String diagnostic) {
    super(MapLibreStatus.UNSUPPORTED, nativeStatusCode, diagnostic);
  }
}
