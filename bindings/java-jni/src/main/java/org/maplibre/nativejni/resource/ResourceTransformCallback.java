package org.maplibre.nativejni.resource;

import java.util.Optional;

/**
 * Rewrites network resource URLs for a runtime.
 *
 * <p>Native code may invoke this callback on worker or network threads. The callback should return
 * quickly and avoid calling Maplibre APIs. Returning {@link Optional#empty()} keeps the original
 * URL. The binding catches callback exceptions and treats them as no rewrite.
 */
@FunctionalInterface
public interface ResourceTransformCallback {
  Optional<String> transform(ResourceTransformRequest request);
}
