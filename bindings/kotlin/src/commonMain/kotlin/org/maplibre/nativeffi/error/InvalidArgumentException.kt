package org.maplibre.nativeffi.error

/** Error for invalid arguments rejected by the native C ABI. */
public class InvalidArgumentException
internal constructor(nativeStatusCode: Int, diagnostic: String = "") :
  MaplibreException(MaplibreStatus.INVALID_ARGUMENT, nativeStatusCode, diagnostic)
