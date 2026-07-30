package org.maplibre.nativeffi.internal.lifecycle

/**
 * Handle values for tests that exercise binding-owned bookkeeping without a live native object.
 *
 * Each value carries the kind byte the C API assigns to the type it stands in for, so a synthetic
 * handle that reaches a diagnostic reads as an obviously fabricated handle of the right kind rather
 * than a plausible one. Passing one to the C API is rejected as a handle this process never
 * created.
 */
internal object SyntheticHandles {
  fun runtime(ordinal: Long = 1): NativeRuntime = NativeRuntime(kind(0x01) or ordinal)

  fun map(ordinal: Long = 1): NativeMap = NativeMap(kind(0x02) or ordinal)

  fun mapProjection(ordinal: Long = 1): NativeMapProjection =
    NativeMapProjection(kind(0x03) or ordinal)

  fun renderSession(ordinal: Long = 1): NativeRenderSession =
    NativeRenderSession(kind(0x04) or ordinal)

  fun offlineRegionSnapshot(ordinal: Long = 1): NativeOfflineRegionSnapshot =
    NativeOfflineRegionSnapshot(kind(0x05) or ordinal)

  fun offlineRegionList(ordinal: Long = 1): NativeOfflineRegionList =
    NativeOfflineRegionList(kind(0x06) or ordinal)

  fun jsonSnapshot(ordinal: Long = 1): NativeJsonSnapshot =
    NativeJsonSnapshot(kind(0x07) or ordinal)

  fun styleIdList(ordinal: Long = 1): NativeStyleIdList = NativeStyleIdList(kind(0x08) or ordinal)

  fun featureQueryResult(ordinal: Long = 1): NativeFeatureQueryResult =
    NativeFeatureQueryResult(kind(0x09) or ordinal)

  fun featureExtensionResult(ordinal: Long = 1): NativeFeatureExtensionResult =
    NativeFeatureExtensionResult(kind(0x0A) or ordinal)

  fun wakeSource(ordinal: Long = 1): NativeWakeSource = NativeWakeSource(kind(0x0B) or ordinal)

  fun resourceRequest(ordinal: Long = 1): NativeResourceRequest =
    NativeResourceRequest(kind(0x0C) or ordinal)

  private fun kind(value: Int): Long = value.toLong() shl 56
}
