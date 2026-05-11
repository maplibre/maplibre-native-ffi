package org.maplibre.nativeffi.error;

/** Thrown when an owner-thread-affine native handle is used from the wrong thread. */
public final class WrongThreadException extends MaplibreException {
  public WrongThreadException(int nativeStatusCode, String diagnostic) {
    super(MaplibreStatus.WRONG_THREAD, nativeStatusCode, diagnostic);
  }
}
