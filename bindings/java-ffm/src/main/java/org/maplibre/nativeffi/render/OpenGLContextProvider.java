package org.maplibre.nativeffi.render;

import java.util.EnumSet;

/** OpenGL context provider support flag reported by the native library build. */
public enum OpenGLContextProvider {
  WGL(1),
  EGL(1 << 1);

  private final int nativeMask;

  OpenGLContextProvider(int nativeMask) {
    this.nativeMask = nativeMask;
  }

  public int nativeMask() {
    return nativeMask;
  }

  public static EnumSet<OpenGLContextProvider> fromMask(int mask) {
    var providers = EnumSet.noneOf(OpenGLContextProvider.class);
    for (var provider : values()) {
      if ((mask & provider.nativeMask) != 0) {
        providers.add(provider);
      }
    }
    return providers;
  }
}
