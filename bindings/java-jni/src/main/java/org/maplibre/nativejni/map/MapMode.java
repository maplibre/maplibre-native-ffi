package org.maplibre.nativejni.map;

/** Map rendering mode selected at map creation. */
public enum MapMode {
  CONTINUOUS(0),
  STATIC(1),
  TILE(2);

  private final int nativeValue;

  MapMode(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }
}
