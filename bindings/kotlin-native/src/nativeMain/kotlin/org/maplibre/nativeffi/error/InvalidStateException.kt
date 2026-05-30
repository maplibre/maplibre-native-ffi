package org.maplibre.nativeffi.error

/** Error for valid objects used in the wrong lifecycle state. */
public class InvalidStateException(nativeStatusCode: Int, diagnostic: String = "") :
  MaplibreException(MaplibreStatus.INVALID_STATE, nativeStatusCode, diagnostic)
