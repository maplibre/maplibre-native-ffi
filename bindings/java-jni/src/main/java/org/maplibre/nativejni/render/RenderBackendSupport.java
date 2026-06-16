package org.maplibre.nativejni.render;

import java.util.Set;

/** Copied render backend support mask reported by the native library build. */
public record RenderBackendSupport(Set<RenderBackend> backends) {
  public RenderBackendSupport {
    backends = backends == null ? Set.of() : Set.copyOf(backends);
  }

  public Set<RenderBackend> asSet() {
    return backends;
  }

  public boolean contains(RenderBackend backend) {
    return backends.contains(backend);
  }

  public boolean isEmpty() {
    return backends.isEmpty();
  }

  public static RenderBackendSupport fromMask(int mask) {
    return new RenderBackendSupport(RenderBackend.fromMask(mask));
  }
}
