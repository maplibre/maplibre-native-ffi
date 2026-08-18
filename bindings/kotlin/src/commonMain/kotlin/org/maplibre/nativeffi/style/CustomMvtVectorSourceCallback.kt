package org.maplibre.nativeffi.style

import org.maplibre.nativeffi.geo.CanonicalTileId

/** Callback invoked by native custom MVT vector sources for tile fetches and cancels. */
public interface CustomMvtVectorSourceCallback {
  public fun fetchTile(tileId: CanonicalTileId)

  public fun cancelTile(tileId: CanonicalTileId) {}
}
