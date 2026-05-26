package org.maplibre.nativeffi.render

/** Render mode reported by render observer events. */
public enum class RenderMode(public val nativeValue: Int) {
  PARTIAL(0),
  FULL(1),
  UNKNOWN(-1);

  public companion object {
    internal fun fromNative(nativeValue: UInt): RenderMode = fromNative(nativeValue.toInt())

    public fun fromNative(nativeValue: Int): RenderMode =
      entries.firstOrNull { it.nativeValue == nativeValue } ?: UNKNOWN
  }
}
