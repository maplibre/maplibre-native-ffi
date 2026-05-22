package org.maplibre.nativejni.error;

/** Thrown when a native call reports an invalid argument. */
public final class InvalidArgumentException extends MaplibreException {
  public InvalidArgumentException(int nativeStatusCode, String diagnostic) {
    super(MaplibreStatus.INVALID_ARGUMENT, nativeStatusCode, diagnostic);
  }
}
