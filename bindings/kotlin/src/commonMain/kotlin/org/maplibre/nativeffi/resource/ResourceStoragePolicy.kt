package org.maplibre.nativeffi.resource

import kotlin.jvm.JvmInline

/**
 * Resource storage policy copied from a native resource request.
 *
 * This is an open domain: a value may have no named constant here, so a `when` over this type needs
 * an `else` branch. Unknown values keep their raw [nativeValue].
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
