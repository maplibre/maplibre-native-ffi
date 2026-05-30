package org.maplibre.nativeffi.resource

/** Decision returned by a resource provider callback. */
public enum class ResourceProviderDecision(public val nativeValue: Int) {
  PASS_THROUGH(0),
  HANDLE(1),
}
