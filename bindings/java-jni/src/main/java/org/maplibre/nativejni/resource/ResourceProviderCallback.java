package org.maplibre.nativejni.resource;

/**
 * Intercepts network resource requests for a runtime.
 *
 * <p>Native code may invoke this callback on worker or network threads. The callback should return
 * quickly and avoid calling map or runtime APIs. Return {@link ResourceProviderDecision#HANDLE}
 * only when the callback stores or completes the supplied {@link ResourceRequestHandle}; otherwise
 * return {@link ResourceProviderDecision#PASS_THROUGH}. If the callback completes the handle
 * inline, the binding reports the request as handled even when the callback returns pass-through.
 * The binding catches callback exceptions and converts them to provider error responses instead of
 * issuing a fallback network request.
 */
@FunctionalInterface
public interface ResourceProviderCallback {
  ResourceProviderDecision handle(ResourceRequest request, ResourceRequestHandle handle);
}
