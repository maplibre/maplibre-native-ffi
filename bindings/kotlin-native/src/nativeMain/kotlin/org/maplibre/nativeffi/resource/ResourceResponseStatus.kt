package org.maplibre.nativeffi.resource

/** Status for a resource provider response. */
public enum class ResourceResponseStatus(public val nativeValue: Int) {
  OK(0),
  ERROR(1),
  NO_CONTENT(2),
  NOT_MODIFIED(3),
}
