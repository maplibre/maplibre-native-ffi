package org.maplibre.nativeffi.error

/** Error for unsupported platforms, backends, entry points, or requested behavior. */
public class UnsupportedFeatureException
internal constructor(nativeStatusCode: Int, diagnostic: String = "") :
  MaplibreException(MaplibreStatus.UNSUPPORTED, nativeStatusCode, diagnostic)
