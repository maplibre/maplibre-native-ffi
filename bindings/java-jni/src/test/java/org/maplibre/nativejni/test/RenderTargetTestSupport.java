package org.maplibre.nativejni.test;

import org.maplibre.nativejni.map.MapHandle;
import org.maplibre.nativejni.map.MapOptions;
import org.maplibre.nativejni.runtime.RuntimeHandle;

/** Shared helpers for render-target tests that need a small local map. */
public final class RenderTargetTestSupport {
  private RenderTargetTestSupport() {}

  public static MapHandle createSmallMap(RuntimeHandle runtime) {
    return MapHandle.create(runtime, new MapOptions().size(64, 64));
  }
}
