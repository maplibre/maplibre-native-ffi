package org.maplibre.nativeffi.resource

/**
 * Intercepts network resource requests for a runtime.
 *
 * Native code may invoke this callback on worker or network threads. Return HANDLE only when the
 * callback stores or completes the supplied ResourceRequestHandle; otherwise return PASS_THROUGH.
 */
public fun interface ResourceProviderCallback {
  public fun handle(
    request: ResourceRequest,
    handle: ResourceRequestHandle,
  ): ResourceProviderDecision
}
