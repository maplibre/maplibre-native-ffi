package org.maplibre.nativeffi.internal.lifecycle

/**
 * cinterop spells every C API handle as `ULong`, so each handle type converts to that at the C
 * boundary and stays distinct everywhere else.
 */
internal val NativeHandle.rawHandleValue: ULong
  get() = raw.toULong()

/** Builds a handle of one kind from a value cinterop returned. */
internal fun runtimeHandle(value: ULong): NativeRuntime = NativeRuntime(value.toLong())

internal fun mapHandle(value: ULong): NativeMap = NativeMap(value.toLong())

internal fun mapProjectionHandle(value: ULong): NativeMapProjection =
  NativeMapProjection(value.toLong())

internal fun renderSessionHandle(value: ULong): NativeRenderSession =
  NativeRenderSession(value.toLong())

internal fun offlineRegionSnapshotHandle(value: ULong): NativeOfflineRegionSnapshot =
  NativeOfflineRegionSnapshot(value.toLong())

internal fun offlineRegionListHandle(value: ULong): NativeOfflineRegionList =
  NativeOfflineRegionList(value.toLong())

internal fun jsonSnapshotHandle(value: ULong): NativeJsonSnapshot =
  NativeJsonSnapshot(value.toLong())

internal fun styleIdListHandle(value: ULong): NativeStyleIdList = NativeStyleIdList(value.toLong())

internal fun styleStringListHandle(value: ULong): NativeStyleStringList =
  NativeStyleStringList(value.toLong())

internal fun featureQueryResultHandle(value: ULong): NativeFeatureQueryResult =
  NativeFeatureQueryResult(value.toLong())

internal fun featureExtensionResultHandle(value: ULong): NativeFeatureExtensionResult =
  NativeFeatureExtensionResult(value.toLong())

internal fun wakeSourceHandle(value: ULong): NativeWakeSource = NativeWakeSource(value.toLong())

internal fun resourceRequestHandle(value: ULong): NativeResourceRequest =
  NativeResourceRequest(value.toLong())

/**
 * Reads a handle the C API wrote to an out-parameter, rejecting the null handle so a create that
 * reported success but produced nothing is caught here rather than at the next call.
 */
internal inline fun <T : NativeHandle> ULong.asHandle(name: String, build: (ULong) -> T): T {
  require(this != 0uL) { "$name returned the null handle" }
  return build(this)
}
