package org.maplibre.nativeffi.log;

/** Category for a Maplibre Native log record. */
public enum LogEvent {
  GENERAL(0),
  SETUP(1),
  SHADER(2),
  PARSE_STYLE(3),
  PARSE_TILE(4),
  RENDER(5),
  STYLE(6),
  DATABASE(7),
  HTTP_REQUEST(8),
  SPRITE(9),
  IMAGE(10),
  OPENGL(11),
  JNI(12),
  ANDROID(13),
  CRASH(14),
  GLYPH(15),
  TIMING(16),
  UNKNOWN(-1);

  private final int nativeValue;

  LogEvent(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }

  public static LogEvent fromNative(int nativeValue) {
    for (var event : values()) {
      if (event.nativeValue == nativeValue) {
        return event;
      }
    }
    return UNKNOWN;
  }
}
