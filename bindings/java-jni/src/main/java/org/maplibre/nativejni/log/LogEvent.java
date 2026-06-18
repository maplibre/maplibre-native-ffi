package org.maplibre.nativejni.log;

/** Category for a Maplibre Native log record. */
public final class LogEvent {
  public static final LogEvent GENERAL = new LogEvent(0, "GENERAL");
  public static final LogEvent SETUP = new LogEvent(1, "SETUP");
  public static final LogEvent SHADER = new LogEvent(2, "SHADER");
  public static final LogEvent PARSE_STYLE = new LogEvent(3, "PARSE_STYLE");
  public static final LogEvent PARSE_TILE = new LogEvent(4, "PARSE_TILE");
  public static final LogEvent RENDER = new LogEvent(5, "RENDER");
  public static final LogEvent STYLE = new LogEvent(6, "STYLE");
  public static final LogEvent DATABASE = new LogEvent(7, "DATABASE");
  public static final LogEvent HTTP_REQUEST = new LogEvent(8, "HTTP_REQUEST");
  public static final LogEvent SPRITE = new LogEvent(9, "SPRITE");
  public static final LogEvent IMAGE = new LogEvent(10, "IMAGE");
  public static final LogEvent OPENGL = new LogEvent(11, "OPENGL");
  public static final LogEvent JNI = new LogEvent(12, "JNI");
  public static final LogEvent ANDROID = new LogEvent(13, "ANDROID");
  public static final LogEvent CRASH = new LogEvent(14, "CRASH");
  public static final LogEvent GLYPH = new LogEvent(15, "GLYPH");
  public static final LogEvent TIMING = new LogEvent(16, "TIMING");

  private final int nativeValue;
  private final String name;

  private LogEvent(int nativeValue, String name) {
    this.nativeValue = nativeValue;
    this.name = name;
  }

  public int nativeValue() {
    return nativeValue;
  }

  public int rawValue() {
    return nativeValue;
  }

  public static LogEvent fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> GENERAL;
      case 1 -> SETUP;
      case 2 -> SHADER;
      case 3 -> PARSE_STYLE;
      case 4 -> PARSE_TILE;
      case 5 -> RENDER;
      case 6 -> STYLE;
      case 7 -> DATABASE;
      case 8 -> HTTP_REQUEST;
      case 9 -> SPRITE;
      case 10 -> IMAGE;
      case 11 -> OPENGL;
      case 12 -> JNI;
      case 13 -> ANDROID;
      case 14 -> CRASH;
      case 15 -> GLYPH;
      case 16 -> TIMING;
      default -> new LogEvent(nativeValue, null);
    };
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof LogEvent value && nativeValue == value.nativeValue;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(nativeValue);
  }

  @Override
  public String toString() {
    return name != null ? name : "LogEvent(" + nativeValue + ")";
  }
}
