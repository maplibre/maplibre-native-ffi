package org.maplibre.nativeffi.offline

/** Offline region status snapshot copied from native storage. */
public data class OfflineRegionStatus(
  public val downloadState: OfflineRegionDownloadState,
  public val rawDownloadState: Int,
  public val completedResourceCount: Long,
  public val completedResourceSize: Long,
  public val completedTileCount: Long,
  public val requiredTileCount: Long,
  public val completedTileSize: Long,
  public val requiredResourceCount: Long,
  public val requiredResourceCountIsPrecise: Boolean,
  public val complete: Boolean,
)
