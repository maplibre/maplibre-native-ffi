package org.maplibre.nativejni.map;

/** North orientation used by map viewport options. */
public final class NorthOrientation {
  public static final NorthOrientation UP = new NorthOrientation(0, "UP");
  public static final NorthOrientation RIGHT = new NorthOrientation(1, "RIGHT");
  public static final NorthOrientation DOWN = new NorthOrientation(2, "DOWN");
  public static final NorthOrientation LEFT = new NorthOrientation(3, "LEFT");

  private final int nativeValue;
  private final String name;

  private NorthOrientation(int nativeValue, String name) {
    this.nativeValue = nativeValue;
    this.name = name;
  }

  public int nativeValue() {
    if (name == null) {
      throw new IllegalArgumentException("Unknown north orientation cannot be used as an input");
    }
    return nativeValue;
  }

  public int rawValue() {
    return nativeValue;
  }

  public static NorthOrientation fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> UP;
      case 1 -> RIGHT;
      case 2 -> DOWN;
      case 3 -> LEFT;
      default -> new NorthOrientation(nativeValue, null);
    };
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof NorthOrientation value && nativeValue == value.nativeValue;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(nativeValue);
  }

  @Override
  public String toString() {
    return name != null ? name : "NorthOrientation(" + nativeValue + ")";
  }
}
