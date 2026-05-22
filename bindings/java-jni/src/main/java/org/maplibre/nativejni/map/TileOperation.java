package org.maplibre.nativejni.map;

/** Tile operation reported by tile observer events. */
public enum TileOperation {
  REQUESTED_FROM_CACHE(0),
  REQUESTED_FROM_NETWORK(1),
  LOAD_FROM_NETWORK(2),
  LOAD_FROM_CACHE(3),
  START_PARSE(4),
  END_PARSE(5),
  ERROR(6),
  CANCELLED(7),
  NULL_OPERATION(8),
  UNKNOWN(-1);

  private final int nativeValue;

  TileOperation(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public static TileOperation fromNative(int nativeValue) {
    for (var operation : values()) {
      if (operation.nativeValue == nativeValue) {
        return operation;
      }
    }
    return UNKNOWN;
  }
}
