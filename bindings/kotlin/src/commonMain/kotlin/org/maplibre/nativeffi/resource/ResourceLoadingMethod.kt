package org.maplibre.nativeffi.resource

import kotlin.jvm.JvmInline

/**
 * Resource loading method copied from a native resource request.
 *
 * This is an open domain: a value may have no named constant here, so a `when` over this type needs
 * an `else` branch. Unknown values keep their raw [nativeValue].
 */
@JvmInline
public value class ResourceLoadingMethod(public val nativeValue: Int) {
  public companion object {
    public val ALL: ResourceLoadingMethod = ResourceLoadingMethod(0)
    public val CACHE_ONLY: ResourceLoadingMethod = ResourceLoadingMethod(1)
    public val NETWORK_ONLY: ResourceLoadingMethod = ResourceLoadingMethod(2)

    internal fun fromNative(nativeValue: UInt): ResourceLoadingMethod =
      fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): ResourceLoadingMethod =
      ResourceLoadingMethod(nativeValue)
  }
}
