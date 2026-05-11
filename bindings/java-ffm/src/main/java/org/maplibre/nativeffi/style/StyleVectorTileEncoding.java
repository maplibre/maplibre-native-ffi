package org.maplibre.nativeffi.style;

/** Vector tile encoding for vector style sources. */
public enum StyleVectorTileEncoding {
  MVT(0),
  MLT(1);

  private final int nativeValue;

  StyleVectorTileEncoding(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }
}
