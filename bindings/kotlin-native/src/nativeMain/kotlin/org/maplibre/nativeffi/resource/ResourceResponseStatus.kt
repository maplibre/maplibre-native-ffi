package org.maplibre.nativeffi.resource

/** Status for a resource provider response. */
public enum class ResourceResponseStatus(internal val nativeValue: UInt) {
  OK(0U),
  ERROR(1U),
  NO_CONTENT(2U),
  NOT_MODIFIED(3U),
}
