package org.maplibre.nativejni.style;

import org.maplibre.nativejni.geo.CanonicalTileId;

/** Callback invoked by native custom geometry sources for tile fetches and cancels. */
public interface CustomGeometrySourceCallback {
  void fetchTile(CanonicalTileId tileId);

  default void cancelTile(CanonicalTileId tileId) {}
}
