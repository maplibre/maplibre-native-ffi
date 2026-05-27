package org.maplibre.nativejni.render;

/** Render mode reported by render observer events. */
public enum RenderMode {
  PARTIAL(0),
  FULL(1),
  UNKNOWN(-1);

  private final int nativeValue;

  RenderMode(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public static RenderMode fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> PARTIAL;
      case 1 -> FULL;
      default -> UNKNOWN;
    };
  }
}
