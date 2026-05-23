package org.maplibre.nativeffi.resource

/** Decision returned by a resource provider callback. */
public enum class ResourceProviderDecision(public val nativeValue: UInt) {
  PASS_THROUGH(0U),
  HANDLE(1U),
}
