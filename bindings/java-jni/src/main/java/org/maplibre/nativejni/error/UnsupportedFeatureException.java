package org.maplibre.nativejni.error;

/** Thrown when the loaded native library does not support a requested feature. */
public final class UnsupportedFeatureException extends MaplibreException {
  public UnsupportedFeatureException(int nativeStatusCode, String diagnostic) {
    super(MaplibreStatus.UNSUPPORTED, nativeStatusCode, diagnostic);
  }
}
