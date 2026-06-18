package org.maplibre.nativejni.map;

/** Viewport orientation mode used by map viewport options. */
public final class ViewportMode {
  public static final ViewportMode DEFAULT = new ViewportMode(0, "DEFAULT");
  public static final ViewportMode FLIPPED_Y = new ViewportMode(1, "FLIPPED_Y");

  private final int nativeValue;
  private final String name;

  private ViewportMode(int nativeValue, String name) {
    this.nativeValue = nativeValue;
    this.name = name;
  }

  public int nativeValue() {
    if (name == null) {
      throw new IllegalArgumentException("Unknown viewport mode cannot be used as an input");
    }
    return nativeValue;
  }

  public int rawValue() {
    return nativeValue;
  }

  public static ViewportMode fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> DEFAULT;
      case 1 -> FLIPPED_Y;
      default -> new ViewportMode(nativeValue, null);
    };
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof ViewportMode value && nativeValue == value.nativeValue;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(nativeValue);
  }

  @Override
  public String toString() {
    return name != null ? name : "ViewportMode(" + nativeValue + ")";
  }
}
