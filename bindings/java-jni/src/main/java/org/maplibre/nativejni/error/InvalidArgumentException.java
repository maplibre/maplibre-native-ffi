package org.maplibre.nativejni.error;

/** Thrown when native code rejects an argument shape, value, or handle. */
public final class InvalidArgumentException extends MaplibreException {
  public InvalidArgumentException(int nativeStatusCode, String diagnostic) {
    super(MaplibreStatus.INVALID_ARGUMENT, nativeStatusCode, diagnostic);
  }
}
