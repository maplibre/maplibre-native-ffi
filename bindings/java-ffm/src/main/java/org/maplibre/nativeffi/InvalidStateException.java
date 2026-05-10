package org.maplibre.nativeffi;

/** Thrown when a native object is in the wrong lifecycle state for a call. */
public final class InvalidStateException extends MapLibreException {
  public InvalidStateException(int nativeStatusCode, String diagnostic) {
    super(MapLibreStatus.INVALID_STATE, nativeStatusCode, diagnostic);
  }
}
