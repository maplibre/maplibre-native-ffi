package org.maplibre.nativeffi.resource

import kotlin.jvm.JvmInline

/** Resource kind reported to runtime resource callbacks. */
@JvmInline
public value class ResourceKind(public val nativeValue: Int) {
  public companion object {
    public val UNKNOWN: ResourceKind = ResourceKind(0)
    public val STYLE: ResourceKind = ResourceKind(1)
    public val SOURCE: ResourceKind = ResourceKind(2)
    public val TILE: ResourceKind = ResourceKind(3)
    public val GLYPHS: ResourceKind = ResourceKind(4)
    public val SPRITE_IMAGE: ResourceKind = ResourceKind(5)
    public val SPRITE_JSON: ResourceKind = ResourceKind(6)
    public val IMAGE: ResourceKind = ResourceKind(7)

    internal fun fromNative(nativeValue: UInt): ResourceKind = fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): ResourceKind = ResourceKind(nativeValue)
  }
}
