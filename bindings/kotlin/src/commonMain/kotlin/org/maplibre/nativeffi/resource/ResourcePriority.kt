package org.maplibre.nativeffi.resource

import kotlin.jvm.JvmInline

/** Resource request priority copied from a native resource request. */
@JvmInline
public value class ResourcePriority(public val nativeValue: Int) {
  public companion object {
    public val REGULAR: ResourcePriority = ResourcePriority(0)
    public val LOW: ResourcePriority = ResourcePriority(1)

    internal fun fromNative(nativeValue: UInt): ResourcePriority = fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): ResourcePriority = ResourcePriority(nativeValue)
  }
}
