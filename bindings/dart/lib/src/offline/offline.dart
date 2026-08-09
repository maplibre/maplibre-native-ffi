/// Offline region definitions, status values, metadata, and operation handles.
library;

import 'dart:typed_data';

import '../geo/geo.dart';

/// Ambient cache maintenance operation.
final class AmbientCacheOperation {
  const AmbientCacheOperation._(this.rawValue, this.name);

  /// Reset the ambient cache database.
  static const resetDatabase = AmbientCacheOperation._(1, 'resetDatabase');

  /// Pack the ambient cache database.
  static const packDatabase = AmbientCacheOperation._(2, 'packDatabase');

  /// Invalidate cached ambient resources.
  static const invalidate = AmbientCacheOperation._(3, 'invalidate');

  /// Clear cached ambient resources.
  static const clear = AmbientCacheOperation._(4, 'clear');

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;
}

/// Offline region download state.
final class OfflineRegionDownloadState {
  const OfflineRegionDownloadState._(this.rawValue, this.name);

  /// Region download is inactive.
  static const inactive = OfflineRegionDownloadState._(0, 'inactive');

  /// Region download is active.
  static const active = OfflineRegionDownloadState._(1, 'active');

  /// Creates a download state from a native raw value.
  factory OfflineRegionDownloadState.fromRawValue(int rawValue) =>
      switch (rawValue) {
        0 => inactive,
        1 => active,
        _ => OfflineRegionDownloadState._(rawValue, 'unknown($rawValue)'),
      };

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;
}

/// Offline region status snapshot.
final class OfflineRegionStatus {
  /// Creates an offline region status snapshot.
  const OfflineRegionStatus({
    required this.downloadState,
    required this.completedResourceCount,
    required this.completedResourceSize,
    required this.completedTileCount,
    required this.requiredTileCount,
    required this.completedTileSize,
    required this.requiredResourceCount,
    required this.requiredResourceCountIsPrecise,
    required this.complete,
  });

  /// Native download state.
  final OfflineRegionDownloadState downloadState;

  /// Completed resource count.
  final BigInt completedResourceCount;

  /// Completed resource byte size.
  final BigInt completedResourceSize;

  /// Completed tile count.
  final BigInt completedTileCount;

  /// Required tile count.
  final BigInt requiredTileCount;

  /// Completed tile byte size.
  final BigInt completedTileSize;

  /// Required resource count.
  final BigInt requiredResourceCount;

  /// Whether [requiredResourceCount] is precise.
  final bool requiredResourceCountIsPrecise;

  /// Whether the region is fully downloaded.
  final bool complete;
}

/// Offline region definition.
sealed class OfflineRegionDefinition {
  const OfflineRegionDefinition();
}

/// Offline region definition type unknown to this binding version.
final class UnknownOfflineRegionDefinition extends OfflineRegionDefinition {
  /// Creates an unknown definition while preserving its native tag.
  const UnknownOfflineRegionDefinition(this.rawType);

  /// Raw native definition type.
  final int rawType;
}

/// Tile-pyramid offline region definition.
final class OfflineTilePyramidRegionDefinition extends OfflineRegionDefinition {
  /// Creates a tile-pyramid offline region definition.
  const OfflineTilePyramidRegionDefinition({
    required this.styleUrl,
    required this.bounds,
    required this.minZoom,
    required this.maxZoom,
    required this.pixelRatio,
    this.includeIdeographs = false,
  });

  /// Style URL copied during region creation.
  final String styleUrl;

  /// Geographic bounds to download.
  final LatLngBounds bounds;

  /// Minimum zoom.
  final double minZoom;

  /// Maximum zoom.
  final double maxZoom;

  /// Pixel ratio.
  final double pixelRatio;

  /// Whether ideographs are included.
  final bool includeIdeographs;
}

/// Geometry offline region definition.
final class OfflineGeometryRegionDefinition extends OfflineRegionDefinition {
  /// Creates a geometry offline region definition.
  const OfflineGeometryRegionDefinition({
    required this.styleUrl,
    required this.geometry,
    required this.minZoom,
    required this.maxZoom,
    required this.pixelRatio,
    this.includeIdeographs = false,
  });

  /// Style URL copied during region creation.
  final String styleUrl;

  /// Geometry to download.
  final Uint8List geometry;

  /// Minimum zoom.
  final double minZoom;

  /// Maximum zoom.
  final double maxZoom;

  /// Pixel ratio.
  final double pixelRatio;

  /// Whether ideographs are included.
  final bool includeIdeographs;
}

/// Copied offline region metadata and definition.
final class OfflineRegionInfo {
  /// Creates copied offline region info.
  OfflineRegionInfo({
    required this.id,
    required this.definition,
    required Uint8List metadata,
  }) : metadata = Uint8List.fromList(metadata).asUnmodifiableView();

  /// Native region identifier.
  final int id;

  /// Copied region definition.
  final OfflineRegionDefinition definition;

  /// Copied opaque metadata bytes.
  final Uint8List metadata;
}
