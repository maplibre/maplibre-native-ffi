package org.maplibre.nativeffi;

/** Thrown when an owner-thread-affine native handle is used from the wrong thread. */
public final class WrongThreadException extends MapLibreException {
  public WrongThreadException(int nativeStatusCode, String diagnostic) {
    super(MapLibreStatus.WRONG_THREAD, nativeStatusCode, diagnostic);
  }
}
