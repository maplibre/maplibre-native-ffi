package org.maplibre.nativeffi.resource;

/** Resource kind reported to runtime resource callbacks. */
public final class ResourceKind {
  public static final ResourceKind UNKNOWN = new ResourceKind(0, "UNKNOWN");
  public static final ResourceKind STYLE = new ResourceKind(1, "STYLE");
  public static final ResourceKind SOURCE = new ResourceKind(2, "SOURCE");
  public static final ResourceKind TILE = new ResourceKind(3, "TILE");
  public static final ResourceKind GLYPHS = new ResourceKind(4, "GLYPHS");
  public static final ResourceKind SPRITE_IMAGE = new ResourceKind(5, "SPRITE_IMAGE");
  public static final ResourceKind SPRITE_JSON = new ResourceKind(6, "SPRITE_JSON");
  public static final ResourceKind IMAGE = new ResourceKind(7, "IMAGE");

  private final int nativeValue;
  private final String name;

  private ResourceKind(int nativeValue, String name) {
    this.nativeValue = nativeValue;
    this.name = name;
  }

  public int nativeValue() {
    if (name == null) {
      throw new IllegalArgumentException("Unknown resource kind cannot be used as an input");
    }
    return nativeValue;
  }

  public int rawValue() {
    return nativeValue;
  }

  public static ResourceKind fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> UNKNOWN;
      case 1 -> STYLE;
      case 2 -> SOURCE;
      case 3 -> TILE;
      case 4 -> GLYPHS;
      case 5 -> SPRITE_IMAGE;
      case 6 -> SPRITE_JSON;
      case 7 -> IMAGE;
      default -> new ResourceKind(nativeValue, null);
    };
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof ResourceKind value && nativeValue == value.nativeValue;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(nativeValue);
  }

  @Override
  public String toString() {
    return name != null ? name : "ResourceKind(" + nativeValue + ")";
  }
}
