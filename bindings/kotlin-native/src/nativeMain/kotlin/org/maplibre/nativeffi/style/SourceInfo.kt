package org.maplibre.nativeffi.style

/** Fixed metadata for one style source. */
public data class SourceInfo(
  public val type: SourceType,
  public val idSize: ULong,
  public val isVolatile: Boolean,
  public val attribution: String? = null,
)
