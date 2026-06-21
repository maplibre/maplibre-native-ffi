package org.maplibre.nativeffi.error

/** Error for owner-thread-affine native handles called from the wrong thread. */
public class WrongThreadException
internal constructor(nativeStatusCode: Int, diagnostic: String = "") :
  MaplibreException(MaplibreStatus.WRONG_THREAD, nativeStatusCode, diagnostic)
