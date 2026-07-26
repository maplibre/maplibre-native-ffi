package org.maplibre.nativeffi.resource

import kotlin.jvm.JvmInline

/**
 * Resource storage policy copied from a native resource request.
 *
 * This is an open domain: MapLibre Native may report a value that has no named constant here, so a
 * `when` over this type needs an `else` branch. Unknown values are preserved as their raw
 * [nativeValue] rather than collapsed to a known constant.
 */
@JvmInline
public value class ResourceStoragePolicy(public val nativeValue: Int) {
  public companion object {
    public val PERMANENT: ResourceStoragePolicy = ResourceStoragePolicy(0)
    public val VOLATILE: ResourceStoragePolicy = ResourceStoragePolicy(1)

    internal fun fromNative(nativeValue: UInt): ResourceStoragePolicy =
      fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): ResourceStoragePolicy =
      ResourceStoragePolicy(nativeValue)
  }
}
