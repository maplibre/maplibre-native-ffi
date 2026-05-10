package org.maplibre.nativeffi;

/** Viewport orientation mode used by map viewport options. */
public enum ViewportMode {
  DEFAULT(0),
  FLIPPED_Y(1),
  UNKNOWN(-1);

  private final int nativeValue;

  ViewportMode(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    if (this == UNKNOWN) {
      throw new IllegalArgumentException("UNKNOWN viewport mode cannot be used as an input");
    }
    return nativeValue;
  }

  public static ViewportMode fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> DEFAULT;
      case 1 -> FLIPPED_Y;
      default -> UNKNOWN;
    };
  }
}
