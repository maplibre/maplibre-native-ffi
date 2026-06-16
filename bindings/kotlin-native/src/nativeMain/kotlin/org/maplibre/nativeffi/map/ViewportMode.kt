package org.maplibre.nativeffi.map

import kotlin.jvm.JvmInline

/** Viewport orientation mode used by map viewport options. */
@JvmInline
public value class ViewportMode(public val nativeValue: Int) {
  public companion object {
    public val DEFAULT: ViewportMode = ViewportMode(0)
    public val FLIPPED_Y: ViewportMode = ViewportMode(1)

    internal fun fromNative(nativeValue: UInt): ViewportMode = fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): ViewportMode = ViewportMode(nativeValue)
  }

  internal val isKnown: Boolean
    get() = this == DEFAULT || this == FLIPPED_Y
}
