package org.maplibre.nativeffi;

/** Canonical tile identity used by custom geometry source callbacks. */
public record CanonicalTileId(int z, long x, long y) {
  public CanonicalTileId {
    if (z < 0 || z > 32) {
      throw new IllegalArgumentException("z must be in [0, 32]");
    }
    var limit = 1L << z;
    if (x < 0 || y < 0 || x >= limit || y >= limit) {
      throw new IllegalArgumentException("x and y must be within zoom bounds");
    }
  }
}
