package org.maplibre.nativeffi.geo;

/** Overscaled tile identity copied from native event payloads. */
public record TileId(
    long overscaledZ, int wrap, long canonicalZ, long canonicalX, long canonicalY) {}
