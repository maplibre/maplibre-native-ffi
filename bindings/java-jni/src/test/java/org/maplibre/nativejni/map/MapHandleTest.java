package org.maplibre.nativejni.map;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertSame;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.error.InvalidStateException;
import org.maplibre.nativejni.internal.access.InternalAccess;
import org.maplibre.nativejni.runtime.RuntimeHandle;
import org.maplibre.nativejni.test.NativeTestSupport;

class MapHandleTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibraryOrSkip();
  }

  @Test
  void createMapKeepsRuntimeAndClosesOnce() {
    try (var runtime = RuntimeHandle.create()) {
      var map = MapHandle.create(runtime, new MapOptions().size(64, 64));

      assertFalse(map.isClosed());
      assertSame(runtime, map.runtime());
      assertTrue(map.nativeAddress(InternalAccess.INSTANCE) != 0);

      map.close();
      assertTrue(map.isClosed());
      map.close();
      assertThrows(InvalidStateException.class, () -> map.nativeAddress(InternalAccess.INSTANCE));
    }
  }

  @Test
  void basicStyleAndRenderRequestsCrossNativeBoundary() {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}");
        map.setStyleUrl("https://example.com/style.json");
        map.requestRepaint();
        assertThrows(InvalidStateException.class, map::requestStillImage);
      }
    }
  }
}
