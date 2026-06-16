package org.maplibre.nativeffi.map;

/** Tile operation reported by tile observer events. */
public final class TileOperation {
  public static final TileOperation REQUESTED_FROM_CACHE =
      new TileOperation(0, "REQUESTED_FROM_CACHE");
  public static final TileOperation REQUESTED_FROM_NETWORK =
      new TileOperation(1, "REQUESTED_FROM_NETWORK");
  public static final TileOperation LOAD_FROM_NETWORK = new TileOperation(2, "LOAD_FROM_NETWORK");
  public static final TileOperation LOAD_FROM_CACHE = new TileOperation(3, "LOAD_FROM_CACHE");
  public static final TileOperation START_PARSE = new TileOperation(4, "START_PARSE");
  public static final TileOperation END_PARSE = new TileOperation(5, "END_PARSE");
  public static final TileOperation ERROR = new TileOperation(6, "ERROR");
  public static final TileOperation CANCELLED = new TileOperation(7, "CANCELLED");
  public static final TileOperation NULL_OPERATION = new TileOperation(8, "NULL_OPERATION");

  private final int nativeValue;
  private final String name;

  private TileOperation(int nativeValue, String name) {
    this.nativeValue = nativeValue;
    this.name = name;
  }

  public int rawValue() {
    return nativeValue;
  }

  public static TileOperation fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> REQUESTED_FROM_CACHE;
      case 1 -> REQUESTED_FROM_NETWORK;
      case 2 -> LOAD_FROM_NETWORK;
      case 3 -> LOAD_FROM_CACHE;
      case 4 -> START_PARSE;
      case 5 -> END_PARSE;
      case 6 -> ERROR;
      case 7 -> CANCELLED;
      case 8 -> NULL_OPERATION;
      default -> new TileOperation(nativeValue, null);
    };
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof TileOperation value && nativeValue == value.nativeValue;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(nativeValue);
  }

  @Override
  public String toString() {
    return name != null ? name : "TileOperation(" + nativeValue + ")";
  }
}
