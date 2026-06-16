package org.maplibre.nativejni.render;

/** Render mode reported by render observer events. */
public final class RenderMode {
  public static final RenderMode PARTIAL = new RenderMode(0, "PARTIAL");
  public static final RenderMode FULL = new RenderMode(1, "FULL");

  private final int nativeValue;
  private final String name;

  private RenderMode(int nativeValue, String name) {
    this.nativeValue = nativeValue;
    this.name = name;
  }

  public int rawValue() {
    return nativeValue;
  }

  public static RenderMode fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> PARTIAL;
      case 1 -> FULL;
      default -> new RenderMode(nativeValue, null);
    };
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof RenderMode value && nativeValue == value.nativeValue;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(nativeValue);
  }

  @Override
  public String toString() {
    return name != null ? name : "RenderMode(" + nativeValue + ")";
  }
}
