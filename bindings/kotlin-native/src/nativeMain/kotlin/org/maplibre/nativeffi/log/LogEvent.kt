package org.maplibre.nativeffi.log

/** Category for a Maplibre Native log record. */
public enum class LogEvent(internal val nativeValue: UInt) {
  GENERAL(0U),
  SETUP(1U),
  SHADER(2U),
  PARSE_STYLE(3U),
  PARSE_TILE(4U),
  RENDER(5U),
  STYLE(6U),
  DATABASE(7U),
  HTTP_REQUEST(8U),
  SPRITE(9U),
  IMAGE(10U),
  OPENGL(11U),
  JNI(12U),
  ANDROID(13U),
  CRASH(14U),
  GLYPH(15U),
  TIMING(16U),
  UNKNOWN(UInt.MAX_VALUE);

  public companion object {
    public fun fromNative(nativeValue: UInt): LogEvent =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
