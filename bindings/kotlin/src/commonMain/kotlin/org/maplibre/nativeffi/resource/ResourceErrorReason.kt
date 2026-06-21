package org.maplibre.nativeffi.resource

import kotlin.jvm.JvmInline

/** Resource error reason copied from native events. */
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
