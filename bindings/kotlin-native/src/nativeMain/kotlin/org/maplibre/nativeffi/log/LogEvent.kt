package org.maplibre.nativeffi.log

/** Category for a Maplibre Native log record. */
public enum class LogEvent(public val nativeValue: Int) {
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

  public companion object {
    internal fun fromNative(nativeValue: UInt): LogEvent = fromNative(nativeValue.toInt())

    public fun fromNative(nativeValue: Int): LogEvent =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
