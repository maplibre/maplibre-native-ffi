package org.maplibre.nativeffi.render

import kotlin.jvm.JvmInline

/**
 * Render mode reported by render observer events.
 *
 * This is an open domain: a value may have no named constant here, so a `when` over this type needs
 * an `else` branch. Unknown values keep their raw [nativeValue].
 */
@JvmInline
public value class RenderMode(public val nativeValue: Int) {
  public companion object {
    public val PARTIAL: RenderMode = RenderMode(0)
    public val FULL: RenderMode = RenderMode(1)

    internal fun fromNative(nativeValue: UInt): RenderMode = fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): RenderMode = RenderMode(nativeValue)
  }
}
