package org.maplibre.nativeffi.offline

/** Offline region status snapshot copied from native storage. */
public data class OfflineRegionStatus(
  public val downloadState: OfflineRegionDownloadState,
  public val completedResourceCount: ULong,
  public val completedResourceSize: ULong,
  public val completedTileCount: ULong,
  public val requiredTileCount: ULong,
  public val completedTileSize: ULong,
  public val requiredResourceCount: ULong,
  public val requiredResourceCountIsPrecise: Boolean,
  public val complete: Boolean,
)
