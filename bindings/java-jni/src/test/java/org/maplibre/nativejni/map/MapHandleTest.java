package org.maplibre.nativejni.map;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertSame;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.EnumSet;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.error.InvalidStateException;
import org.maplibre.nativejni.geo.ScreenPoint;
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
  void debugAndLoadingStateCrossNativeBoundary() {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        map.setDebugOptions(EnumSet.of(DebugOption.TILE_BORDERS, DebugOption.COLLISION));
        assertEquals(
            EnumSet.of(DebugOption.TILE_BORDERS, DebugOption.COLLISION), map.debugOptions());
        map.setDebugOptions(EnumSet.noneOf(DebugOption.class));
        assertEquals(EnumSet.noneOf(DebugOption.class), map.debugOptions());

        assertFalse(map.isRenderingStatsViewEnabled());
        map.setRenderingStatsViewEnabled(true);
        assertTrue(map.isRenderingStatsViewEnabled());
        map.setRenderingStatsViewEnabled(false);
        assertFalse(map.isRenderingStatsViewEnabled());

        assertFalse(map.isFullyLoaded());
        map.dumpDebugLogs();
      }
    }
  }

  @Test
  void primitiveCameraCommandsCrossNativeBoundary() {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        map.moveBy(1, 2);
        map.moveByAnimated(0, 0);
        map.scaleBy(1.1);
        map.scaleBy(1.0, new ScreenPoint(32, 32));
        map.scaleByAnimated(1.0);
        map.scaleByAnimated(1.0, new ScreenPoint(32, 32));
        map.rotateBy(new ScreenPoint(0, 0), new ScreenPoint(1, 1));
        map.rotateByAnimated(new ScreenPoint(0, 0), new ScreenPoint(1, 1));
        map.pitchBy(0);
        map.pitchByAnimated(0);
        map.cancelTransitions();
      }
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
