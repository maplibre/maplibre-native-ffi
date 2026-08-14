package org.maplibre.nativeffi.runtime

import kotlin.jvm.JvmInline

/** Internal typed-result dispatch classification for an offline operation. */
@JvmInline
internal value class OperationKind(internal val nativeValue: Int) {
  internal companion object {
    internal val AMBIENT_CACHE: OperationKind = OperationKind(1)
    internal val REGION_CREATE: OperationKind = OperationKind(2)
    internal val REGION_GET: OperationKind = OperationKind(3)
    internal val REGIONS_LIST: OperationKind = OperationKind(4)
    internal val REGIONS_MERGE_DATABASE: OperationKind = OperationKind(5)
    internal val REGION_UPDATE_METADATA: OperationKind = OperationKind(6)
    internal val REGION_GET_STATUS: OperationKind = OperationKind(7)
    internal val REGION_SET_OBSERVED: OperationKind = OperationKind(8)
    internal val REGION_SET_DOWNLOAD_STATE: OperationKind = OperationKind(9)
    internal val REGION_INVALIDATE: OperationKind = OperationKind(10)
    internal val REGION_DELETE: OperationKind = OperationKind(11)
    internal val SET_MAXIMUM_AMBIENT_CACHE_SIZE: OperationKind = OperationKind(12)
    internal val RENDER_ATTACH: OperationKind = OperationKind(20)
    internal val RENDER_CONTROL: OperationKind = OperationKind(21)
    internal val RENDER_QUERY: OperationKind = OperationKind(22)
    internal val RENDER_READBACK: OperationKind = OperationKind(23)
    internal val FRAME_RELEASE: OperationKind = OperationKind(24)
    internal val RENDER_FEATURE_STATE_GET: OperationKind = OperationKind(25)

    internal fun fromNative(nativeValue: UInt): OperationKind = fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): OperationKind = OperationKind(nativeValue)
  }
}
