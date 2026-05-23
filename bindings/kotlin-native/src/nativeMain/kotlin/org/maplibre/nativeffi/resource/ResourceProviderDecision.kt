package org.maplibre.nativeffi.resource

/** Decision returned by a resource provider callback. */
public enum class ResourceProviderDecision(internal val nativeValue: UInt) {
  PASS_THROUGH(0U),
  HANDLE(1U),
}
