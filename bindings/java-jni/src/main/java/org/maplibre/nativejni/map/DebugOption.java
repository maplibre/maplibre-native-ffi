package org.maplibre.nativejni.map;

/** Map debug overlay options. */
public enum DebugOption {
  TILE_BORDERS(1 << 1),
  PARSE_STATUS(1 << 2),
  TIMESTAMPS(1 << 3),
  COLLISION(1 << 4),
  OVERDRAW(1 << 5),
  STENCIL_CLIP(1 << 6),
  DEPTH_BUFFER(1 << 7);

  private final int nativeMask;

  DebugOption(int nativeMask) {
    this.nativeMask = nativeMask;
  }

  public int nativeMask() {
    return nativeMask;
  }
}
