package org.maplibre.nativeffi.map

import kotlin.jvm.JvmInline

/**
 * Constraint mode used by map viewport options.
 *
 * This is an open domain: MapLibre Native may report a value that has no named constant here, so a
 * `when` over this type needs an `else` branch. Unknown values are preserved as their raw
 * [nativeValue] rather than collapsed to a known constant.
 */
@JvmInline
public value class ConstrainMode(public val nativeValue: Int) {
  public companion object {
    public val NONE: ConstrainMode = ConstrainMode(0)
    public val HEIGHT_ONLY: ConstrainMode = ConstrainMode(1)
    public val WIDTH_AND_HEIGHT: ConstrainMode = ConstrainMode(2)
    public val SCREEN: ConstrainMode = ConstrainMode(3)

    internal fun fromNative(nativeValue: UInt): ConstrainMode = fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): ConstrainMode = ConstrainMode(nativeValue)
  }

  internal val isKnown: Boolean
    get() = this == NONE || this == HEIGHT_ONLY || this == WIDTH_AND_HEIGHT || this == SCREEN
}
