package org.maplibre.nativejni.map;

import static org.junit.jupiter.api.Assertions.assertFalse;

import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.runtime.RuntimeHandle;

final class StyleHandleTest {
  @Test
  void bnd101AndBnd105StyleJsonAndSourceQueriesCrossNativeBoundary() {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}");
        assertFalse(map.styleSourceExists("missing-source"));
        assertFalse(map.styleSourceIds().contains("missing-source"));
      }
    }
  }
}
