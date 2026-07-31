package org.maplibre.nativeffi.style

import kotlin.jvm.JvmInline

/**
 * How a stretchable image fits text along one axis.
 *
 * This is an open domain: MapLibre Native may report a value that has no named constant here, so a
 * `when` over this type needs an `else` branch. Unknown values are preserved as their raw
 * [nativeValue] rather than collapsed to a known constant.
 */
@JvmInline
public value class StyleImageTextFit(public val nativeValue: Int) {
  public companion object {
    public val STRETCH_OR_SHRINK: StyleImageTextFit = StyleImageTextFit(0)
    public val STRETCH_ONLY: StyleImageTextFit = StyleImageTextFit(1)
    public val PROPORTIONAL: StyleImageTextFit = StyleImageTextFit(2)

    internal fun fromNative(nativeValue: UInt): StyleImageTextFit = fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): StyleImageTextFit = StyleImageTextFit(nativeValue)
  }
}
