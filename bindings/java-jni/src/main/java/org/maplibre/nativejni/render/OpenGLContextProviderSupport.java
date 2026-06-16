package org.maplibre.nativejni.render;

import java.util.Set;

/** Copied OpenGL context provider support mask reported by the native library build. */
public record OpenGLContextProviderSupport(Set<OpenGLContextProvider> providers) {
  public OpenGLContextProviderSupport {
    providers = providers == null ? Set.of() : Set.copyOf(providers);
  }

  public Set<OpenGLContextProvider> asSet() {
    return providers;
  }

  public boolean contains(OpenGLContextProvider provider) {
    return providers.contains(provider);
  }

  public boolean isEmpty() {
    return providers.isEmpty();
  }

  public static OpenGLContextProviderSupport fromMask(int mask) {
    return new OpenGLContextProviderSupport(OpenGLContextProvider.fromMask(mask));
  }
}
