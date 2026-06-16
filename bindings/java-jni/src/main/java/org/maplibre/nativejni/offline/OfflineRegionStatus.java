package org.maplibre.nativejni.offline;

/** Offline region status snapshot copied from native event payloads. */
public record OfflineRegionStatus(
    OfflineRegionDownloadState downloadState,
    long completedResourceCount,
    long completedResourceSize,
    long completedTileCount,
    long requiredTileCount,
    long completedTileSize,
    long requiredResourceCount,
    boolean requiredResourceCountIsPrecise,
    boolean complete) {}
