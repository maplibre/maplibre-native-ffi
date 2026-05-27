package org.maplibre.nativejni.log;

/** Severity for a Maplibre Native log record. */
public enum LogSeverity {
  INFO(1),
  WARNING(2),
  ERROR(3),
  UNKNOWN(-1);

  private final int nativeValue;

  LogSeverity(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }

  public int nativeMask() {
    if (this == UNKNOWN) {
      throw new IllegalArgumentException("UNKNOWN log severity cannot be used as an input");
    }
    return 1 << nativeValue;
  }

  public static LogSeverity fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 1 -> INFO;
      case 2 -> WARNING;
      case 3 -> ERROR;
      default -> UNKNOWN;
    };
  }
}
