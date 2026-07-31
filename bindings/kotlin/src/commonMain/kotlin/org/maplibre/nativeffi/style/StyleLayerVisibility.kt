package org.maplibre.nativeffi.style

import kotlin.jvm.JvmInline

/**
 * Whether a style layer draws.
 *
 * This is an open domain: MapLibre Native may report a value that has no named constant here, so a
 * `when` over this type needs an `else` branch. Unknown values are preserved as their raw
 * [nativeValue] rather than collapsed to a known constant.
 */
@JvmInline
public value class StyleLayerVisibility(public val nativeValue: Int) {
  public companion object {
    public val VISIBLE: StyleLayerVisibility = StyleLayerVisibility(0)
    public val NONE: StyleLayerVisibility = StyleLayerVisibility(1)

    internal fun fromNative(nativeValue: UInt): StyleLayerVisibility =
      fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): StyleLayerVisibility =
      StyleLayerVisibility(nativeValue)
  }
}
