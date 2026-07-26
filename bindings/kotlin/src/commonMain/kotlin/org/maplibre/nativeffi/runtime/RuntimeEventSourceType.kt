package org.maplibre.nativeffi.runtime

import kotlin.jvm.JvmInline

/**
 * Source kind for a copied runtime event.
 *
 * This is an open domain: MapLibre Native may report a value that has no named constant here, so a
 * `when` over this type needs an `else` branch. Unknown values are preserved as their raw
 * [nativeValue] rather than collapsed to a known constant.
 */
@JvmInline
public value class RuntimeEventSourceType(public val nativeValue: Int) {
  public companion object {
    public val RUNTIME: RuntimeEventSourceType = RuntimeEventSourceType(0)
    public val MAP: RuntimeEventSourceType = RuntimeEventSourceType(1)

    internal fun fromNative(nativeValue: UInt): RuntimeEventSourceType =
      fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): RuntimeEventSourceType =
      RuntimeEventSourceType(nativeValue)
  }
}
