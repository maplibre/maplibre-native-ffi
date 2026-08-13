package org.maplibre.nativeffi.runtime

import kotlin.jvm.JvmInline

/** Internal typed-result shape for an offline operation. */
@JvmInline
internal value class OperationResultKind(internal val nativeValue: Int) {
  internal companion object {
    internal val NONE: OperationResultKind = OperationResultKind(0)
    internal val REGION: OperationResultKind = OperationResultKind(1)
    internal val OPTIONAL_REGION: OperationResultKind = OperationResultKind(2)
    internal val REGION_LIST: OperationResultKind = OperationResultKind(3)
    internal val REGION_STATUS: OperationResultKind = OperationResultKind(4)

    internal fun fromNative(nativeValue: UInt): OperationResultKind =
      fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): OperationResultKind =
      OperationResultKind(nativeValue)
  }
}
