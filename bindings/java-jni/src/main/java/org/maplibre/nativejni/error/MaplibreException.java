package org.maplibre.nativejni.error;

import java.util.Objects;

/** Base unchecked exception for errors reported by the native Maplibre C ABI. */
public class MaplibreException extends RuntimeException {
  private final MaplibreStatus status;
  private final int nativeStatusCode;
  private final String diagnostic;

  public MaplibreException(MaplibreStatus status, int nativeStatusCode, String diagnostic) {
    super(message(status, nativeStatusCode, diagnostic));
    this.status = Objects.requireNonNull(status, "status");
    this.nativeStatusCode = nativeStatusCode;
    this.diagnostic = diagnostic == null ? "" : diagnostic;
  }

  public MaplibreStatus status() {
    return status;
  }

  public int nativeStatusCode() {
    return nativeStatusCode;
  }

  public String diagnostic() {
    return diagnostic;
  }

  public static MaplibreException forStatus(
      MaplibreStatus status, int nativeStatusCode, String diagnostic) {
    if (MaplibreStatus.INVALID_ARGUMENT.equals(status)) {
      return new InvalidArgumentException(nativeStatusCode, diagnostic);
    }
    if (MaplibreStatus.INVALID_STATE.equals(status)) {
      return new InvalidStateException(nativeStatusCode, diagnostic);
    }
    if (MaplibreStatus.WRONG_THREAD.equals(status)) {
      return new WrongThreadException(nativeStatusCode, diagnostic);
    }
    if (MaplibreStatus.UNSUPPORTED.equals(status)) {
      return new UnsupportedFeatureException(nativeStatusCode, diagnostic);
    }
    if (MaplibreStatus.NATIVE_ERROR.equals(status)) {
      return new NativeErrorException(nativeStatusCode, diagnostic);
    }
    return new MaplibreException(status, nativeStatusCode, diagnostic);
  }

  private static String message(MaplibreStatus status, int nativeStatusCode, String diagnostic) {
    var detail =
        diagnostic == null || diagnostic.isBlank() ? "No native diagnostic available." : diagnostic;
    return "%s (%d): %s".formatted(status, nativeStatusCode, detail);
  }
}
