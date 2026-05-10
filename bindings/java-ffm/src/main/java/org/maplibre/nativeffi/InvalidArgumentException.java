package org.maplibre.nativeffi;

/** Thrown when a native call reports an invalid argument. */
public final class InvalidArgumentException extends MapLibreException {
  public InvalidArgumentException(int nativeStatusCode, String diagnostic) {
    super(MapLibreStatus.INVALID_ARGUMENT, nativeStatusCode, diagnostic);
  }
}
