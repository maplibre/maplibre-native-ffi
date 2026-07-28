package org.maplibre.nativeffi.resource

import kotlin.jvm.JvmInline

/**
 * Resource error reason copied from native events.
 *
 * This is an open domain: MapLibre Native may report a value that has no named constant here, so a
 * `when` over this type needs an `else` branch. Unknown values are preserved as their raw
 * [nativeValue] rather than collapsed to a known constant.
 */
@JvmInline
public value class ResourceErrorReason(public val nativeValue: Int) {
  public companion object {
    public val NONE: ResourceErrorReason = ResourceErrorReason(0)
    public val NOT_FOUND: ResourceErrorReason = ResourceErrorReason(1)
    public val SERVER: ResourceErrorReason = ResourceErrorReason(2)
    public val CONNECTION: ResourceErrorReason = ResourceErrorReason(3)
    public val RATE_LIMIT: ResourceErrorReason = ResourceErrorReason(4)
    public val OTHER: ResourceErrorReason = ResourceErrorReason(5)

    internal fun fromNative(nativeValue: UInt): ResourceErrorReason =
      fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): ResourceErrorReason =
      ResourceErrorReason(nativeValue)
  }

  internal val isKnown: Boolean
    get() =
      this == NONE ||
        this == NOT_FOUND ||
        this == SERVER ||
        this == CONNECTION ||
        this == RATE_LIMIT ||
        this == OTHER
}
