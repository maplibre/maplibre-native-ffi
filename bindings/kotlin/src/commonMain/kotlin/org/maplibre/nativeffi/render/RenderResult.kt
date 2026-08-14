package org.maplibre.nativeffi.render

import kotlin.jvm.JvmInline

/**
 * Terminal disposition of an accepted frame demand.
 *
 * This is an open domain: a value may have no named constant here, so a `when` over this type needs
 * an `else` branch. Unknown values keep their raw [nativeValue].
 */
@JvmInline
public value class RenderResult(public val nativeValue: Int) {
  public companion object {
    /** The call rendered a frame into the render target. */
    public val RENDERED: RenderResult = RenderResult(0)

    /** The call produced no frame. Wait for a render-update-available event. */
    public val NO_UPDATE: RenderResult = RenderResult(1)

    /**
     * The map has not applied the session's current size yet. Wait for the next
     * render-update-available event.
     */
    public val SIZE_PENDING: RenderResult = RenderResult(2)

    /**
     * The render target had no frame to draw into. Wait for a host event that changes the render
     * target, or back off and retry.
     */
    public val TARGET_NOT_READY: RenderResult = RenderResult(3)

    /** A newer demand in the same coalescing boundary replaced this demand. */
    public val SUPERSEDED: RenderResult = RenderResult(4)

    /** The demand's positive monotonic deadline elapsed before rendering began. */
    public val DEADLINE_MISSED: RenderResult = RenderResult(5)

    internal fun fromNative(nativeValue: UInt): RenderResult = fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): RenderResult = RenderResult(nativeValue)
  }
}
