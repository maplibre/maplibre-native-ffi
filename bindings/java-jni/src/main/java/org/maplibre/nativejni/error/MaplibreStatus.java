package org.maplibre.nativejni.error;

import java.util.Objects;

/** Status categories reported by the native Maplibre C ABI. */
public final class MaplibreStatus {
  public static final MaplibreStatus OK = new MaplibreStatus(0, "OK");
  public static final MaplibreStatus INVALID_ARGUMENT = new MaplibreStatus(-1, "INVALID_ARGUMENT");
  public static final MaplibreStatus INVALID_STATE = new MaplibreStatus(-2, "INVALID_STATE");
  public static final MaplibreStatus WRONG_THREAD = new MaplibreStatus(-3, "WRONG_THREAD");
  public static final MaplibreStatus UNSUPPORTED = new MaplibreStatus(-4, "UNSUPPORTED");
  public static final MaplibreStatus NATIVE_ERROR = new MaplibreStatus(-5, "NATIVE_ERROR");

  private final int nativeCode;
  private final String name;

  private MaplibreStatus(int nativeCode, String name) {
    this.nativeCode = nativeCode;
    this.name = name;
  }

  /** Returns the raw C ABI status value. */
  public int nativeCode() {
    return nativeCode;
  }

  public static MaplibreStatus fromNative(int nativeCode) {
    return switch (nativeCode) {
      case 0 -> OK;
      case -1 -> INVALID_ARGUMENT;
      case -2 -> INVALID_STATE;
      case -3 -> WRONG_THREAD;
      case -4 -> UNSUPPORTED;
      case -5 -> NATIVE_ERROR;
      default -> new MaplibreStatus(nativeCode, null);
    };
  }

  @Override
  public boolean equals(Object other) {
    return this == other
        || (other instanceof MaplibreStatus status && nativeCode == status.nativeCode);
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(nativeCode);
  }

  @Override
  public String toString() {
    return Objects.requireNonNullElseGet(name, () -> "MaplibreStatus(" + nativeCode + ")");
  }
}
