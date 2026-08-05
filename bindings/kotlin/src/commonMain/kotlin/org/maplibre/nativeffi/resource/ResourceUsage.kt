package org.maplibre.nativeffi.resource

import kotlin.jvm.JvmInline

/**
 * Resource usage copied from a native resource request.
 *
 * This is an open domain: a value may have no named constant here, so a `when` over this type needs
 * an `else` branch. Unknown values keep their raw [nativeValue].
 */
@JvmInline
public value class ResourceUsage(public val nativeValue: Int) {
  public companion object {
    public val ONLINE: ResourceUsage = ResourceUsage(0)
    public val OFFLINE: ResourceUsage = ResourceUsage(1)

    internal fun fromNative(nativeValue: UInt): ResourceUsage = fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): ResourceUsage = ResourceUsage(nativeValue)
  }
}
