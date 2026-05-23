package org.maplibre.nativeffi.render

/** Render mode reported by render observer events. */
public enum class RenderMode(public val nativeValue: UInt) {
  PARTIAL(0U),
  FULL(1U),
  UNKNOWN(UInt.MAX_VALUE);

  public companion object {
    public fun fromNative(nativeValue: UInt): RenderMode =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
