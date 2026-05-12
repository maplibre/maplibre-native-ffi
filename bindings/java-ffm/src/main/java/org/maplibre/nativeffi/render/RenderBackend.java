package org.maplibre.nativeffi.render;

import java.util.EnumSet;

/** Render backend support flag reported by the native library build. */
public enum RenderBackend {
  METAL(1),
  VULKAN(1 << 1),
  OPENGL(1 << 2);

  private final int nativeMask;

  RenderBackend(int nativeMask) {
    this.nativeMask = nativeMask;
  }

  public int nativeMask() {
    return nativeMask;
  }

  public static EnumSet<RenderBackend> fromMask(int mask) {
    var backends = EnumSet.noneOf(RenderBackend.class);
    for (var backend : values()) {
      if ((mask & backend.nativeMask) != 0) {
        backends.add(backend);
      }
    }
    return backends;
  }
}
