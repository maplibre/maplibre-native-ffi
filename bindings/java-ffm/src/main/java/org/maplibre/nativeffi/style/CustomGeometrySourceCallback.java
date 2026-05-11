package org.maplibre.nativeffi.style;

import org.maplibre.nativeffi.geo.CanonicalTileId;

/** Callback invoked by native custom geometry sources for tile fetches and cancels. */
public interface CustomGeometrySourceCallback {
  void fetchTile(CanonicalTileId tileId);

  default void cancelTile(CanonicalTileId tileId) {}
}
