package org.maplibre.nativeffi.internal.lifecycle

/**
 * cinterop spells every C API handle as `ULong`, so each handle type converts to that at the C
 * boundary and stays distinct everywhere else.
 */
internal val NativeHandle.rawHandleValue: ULong
  get() = raw.toULong()

internal fun runtimeHandle(value: ULong): NativeRuntime = NativeRuntime(value.toLong())

internal fun mapHandle(value: ULong): NativeMap = NativeMap(value.toLong())

internal fun mapProjectionHandle(value: ULong): NativeMapProjection =
  NativeMapProjection(value.toLong())

internal fun renderSessionHandle(value: ULong): NativeRenderSession =
  NativeRenderSession(value.toLong())

internal fun resourceRequestHandle(value: ULong): NativeResourceRequest =
  NativeResourceRequest(value.toLong())

internal fun geoJsonSourceDataHandle(value: ULong): NativeGeoJsonSourceData =
  NativeGeoJsonSourceData(value.toLong())

/** Reads a handle the C API wrote to an out-parameter, rejecting the null handle. */
internal inline fun <T : NativeHandle> ULong.asHandle(name: String, build: (ULong) -> T): T {
  require(this != 0uL) { "$name returned the null handle" }
  return build(this)
}
