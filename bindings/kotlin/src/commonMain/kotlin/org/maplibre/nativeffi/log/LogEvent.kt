package org.maplibre.nativeffi.log

import kotlin.jvm.JvmInline

/** Category for a Maplibre Native log record. */
@JvmInline
public value class LogEvent(public val nativeValue: Int) {
  public companion object {
    public val GENERAL: LogEvent = LogEvent(0)
    public val SETUP: LogEvent = LogEvent(1)
    public val SHADER: LogEvent = LogEvent(2)
    public val PARSE_STYLE: LogEvent = LogEvent(3)
    public val PARSE_TILE: LogEvent = LogEvent(4)
    public val RENDER: LogEvent = LogEvent(5)
    public val STYLE: LogEvent = LogEvent(6)
    public val DATABASE: LogEvent = LogEvent(7)
    public val HTTP_REQUEST: LogEvent = LogEvent(8)
    public val SPRITE: LogEvent = LogEvent(9)
    public val IMAGE: LogEvent = LogEvent(10)
    public val OPENGL: LogEvent = LogEvent(11)
    public val JNI: LogEvent = LogEvent(12)
    public val ANDROID: LogEvent = LogEvent(13)
    public val CRASH: LogEvent = LogEvent(14)
    public val GLYPH: LogEvent = LogEvent(15)
    public val TIMING: LogEvent = LogEvent(16)

    internal fun fromNative(nativeValue: UInt): LogEvent = fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): LogEvent = LogEvent(nativeValue)
  }
}
