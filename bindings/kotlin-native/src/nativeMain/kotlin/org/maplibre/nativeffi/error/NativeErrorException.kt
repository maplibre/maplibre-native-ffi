package org.maplibre.nativeffi.error

/** Error for native MapLibre failures or C++ exceptions converted to C status. */
public open class NativeErrorException
internal constructor(nativeStatusCode: Int, diagnostic: String = "") :
  MaplibreException(MaplibreStatus.NATIVE_ERROR, nativeStatusCode, diagnostic)
