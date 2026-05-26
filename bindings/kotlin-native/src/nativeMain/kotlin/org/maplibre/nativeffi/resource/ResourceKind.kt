package org.maplibre.nativeffi.resource

/** Resource kind reported to runtime resource callbacks. */
public enum class ResourceKind(public val nativeValue: Int) {
  UNKNOWN(0),
  STYLE(1),
  SOURCE(2),
  TILE(3),
  GLYPHS(4),
  SPRITE_IMAGE(5),
  SPRITE_JSON(6),
  IMAGE(7);

  public companion object {
    internal fun fromNative(nativeValue: UInt): ResourceKind = fromNative(nativeValue.toInt())

    public fun fromNative(nativeValue: Int): ResourceKind =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
