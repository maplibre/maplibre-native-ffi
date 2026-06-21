package org.maplibre.nativeffi.style

/** Copied fixed metadata for one style source. */
public data class SourceInfo(
  public val type: SourceType,
  public val volatileSource: Boolean,
  public val attribution: String?,
)
