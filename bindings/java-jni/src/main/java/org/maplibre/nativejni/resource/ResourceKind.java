package org.maplibre.nativejni.resource;

/** Resource kind reported to runtime resource callbacks. */
public enum ResourceKind {
  UNKNOWN(0),
  STYLE(1),
  SOURCE(2),
  TILE(3),
  GLYPHS(4),
  SPRITE_IMAGE(5),
  SPRITE_JSON(6),
  IMAGE(7);

  private final int nativeValue;

  ResourceKind(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }

  public static ResourceKind fromNative(int nativeValue) {
    for (var kind : values()) {
      if (kind.nativeValue == nativeValue) {
        return kind;
      }
    }
    return UNKNOWN;
  }
}
