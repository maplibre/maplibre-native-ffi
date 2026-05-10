package org.maplibre.nativeffi;

/** Callback invoked by native custom geometry sources for tile fetches and cancels. */
public interface CustomGeometrySourceCallback {
  void fetchTile(CanonicalTileId tileId);

  default void cancelTile(CanonicalTileId tileId) {}
}
