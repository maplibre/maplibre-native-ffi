package org.maplibre.nativeffi.geo;

/** Canonical tile identity used by custom geometry source callbacks. */
public record CanonicalTileId(int z, long x, long y) {}
