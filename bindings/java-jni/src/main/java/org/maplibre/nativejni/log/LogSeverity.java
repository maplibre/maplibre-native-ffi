package org.maplibre.nativejni.log;

/** Severity for a Maplibre Native log record. */
public final class LogSeverity {
  public static final LogSeverity INFO = new LogSeverity(1, "INFO");
  public static final LogSeverity WARNING = new LogSeverity(2, "WARNING");
  public static final LogSeverity ERROR = new LogSeverity(3, "ERROR");

  private final int nativeValue;
  private final String name;

  private LogSeverity(int nativeValue, String name) {
    this.nativeValue = nativeValue;
    this.name = name;
  }

  public int nativeValue() {
    if (name == null) {
      throw new IllegalArgumentException("Unknown log severity cannot be used as an input");
    }
    return nativeValue;
  }

  public int nativeMask() {
    return 1 << nativeValue();
  }

  public int rawValue() {
    return nativeValue;
  }

  public static LogSeverity fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 1 -> INFO;
      case 2 -> WARNING;
      case 3 -> ERROR;
      default -> new LogSeverity(nativeValue, null);
    };
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof LogSeverity value && nativeValue == value.nativeValue;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(nativeValue);
  }

  @Override
  public String toString() {
    return name != null ? name : "LogSeverity(" + nativeValue + ")";
  }
}
