package org.maplibre.nativejni.error;

/** Thrown when a native object is in the wrong lifecycle state for a call. */
public final class InvalidStateException extends MaplibreException {
  public InvalidStateException(int nativeStatusCode, String diagnostic) {
    super(MaplibreStatus.INVALID_STATE, nativeStatusCode, diagnostic);
  }
}
