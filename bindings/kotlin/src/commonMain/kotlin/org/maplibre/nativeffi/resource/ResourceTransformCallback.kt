package org.maplibre.nativeffi.resource

/**
 * Rewrites network resource URLs for a runtime.
 *
 * Native code may invoke this callback on worker or network threads. The callback should return
 * quickly and avoid calling Maplibre APIs. Returning null keeps the original URL. The binding
 * catches callback exceptions and treats them as no rewrite.
 */
public fun interface ResourceTransformCallback {
  public fun transform(request: ResourceTransformRequest): String?
}
