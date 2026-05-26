package org.maplibre.nativeffi.resource

/** Copied request passed to a runtime resource transform callback. */
public data class ResourceTransformRequest(
  public val kind: ResourceKind,
  public val rawKind: Int,
  public val url: String,
)
