package org.maplibre.nativeffi.resource

/** Resource kind reported to runtime resource callbacks. */
public enum class ResourceKind(internal val nativeValue: UInt) {
  UNKNOWN(0U),
  STYLE(1U),
  SOURCE(2U),
  TILE(3U),
  GLYPHS(4U),
  SPRITE_IMAGE(5U),
  SPRITE_JSON(6U),
  IMAGE(7U);

  public companion object {
    public fun fromNative(nativeValue: UInt): ResourceKind =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
