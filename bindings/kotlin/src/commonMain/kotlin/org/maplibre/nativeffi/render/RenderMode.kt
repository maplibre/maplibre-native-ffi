package org.maplibre.nativeffi.render

import kotlin.jvm.JvmInline

/** Render mode reported by render observer events. */
@JvmInline
public value class RenderMode(public val nativeValue: Int) {
  public companion object {
    public val PARTIAL: RenderMode = RenderMode(0)
    public val FULL: RenderMode = RenderMode(1)

    internal fun fromNative(nativeValue: UInt): RenderMode = fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): RenderMode = RenderMode(nativeValue)
  }
}
