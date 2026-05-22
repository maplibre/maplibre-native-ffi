package org.maplibre.nativejni.map;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertSame;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.EnumSet;
import java.util.List;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.camera.AnimationOptions;
import org.maplibre.nativejni.camera.CameraOptions;
import org.maplibre.nativejni.error.InvalidStateException;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.geo.ScreenPoint;
import org.maplibre.nativejni.internal.access.InternalAccess;
import org.maplibre.nativejni.runtime.RuntimeHandle;
import org.maplibre.nativejni.style.SourceType;
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
  void cameraStateCommandsCrossNativeBoundary() {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        map.jumpTo(new CameraOptions().center(10, 20).zoom(3).bearing(4).pitch(5));
        var camera = map.camera();
        assertTrue(camera.hasCenter());
        assertEquals(10, camera.center().latitude(), 0.000001);
        assertEquals(20, camera.center().longitude(), 0.000001);
        assertTrue(camera.hasZoom());
        assertEquals(3, camera.zoom(), 0.000001);

        var point = map.pixelForLatLng(camera.center());
        assertTrue(Double.isFinite(point.x()));
        assertTrue(Double.isFinite(point.y()));
        var coordinate = map.latLngForPixel(point);
        assertTrue(Double.isFinite(coordinate.latitude()));
        assertTrue(Double.isFinite(coordinate.longitude()));
        var points = map.pixelsForLatLngs(List.of(camera.center(), new LatLng(0, 0)));
        assertEquals(2, points.size());
        var coordinates = map.latLngsForPixels(points);
        assertEquals(2, coordinates.size());

        var animation = new AnimationOptions().durationMs(0);
        map.easeTo(new CameraOptions().zoom(4), animation);
        map.flyTo(new CameraOptions().zoom(3), animation);
      }
    }
  }

  @Test
  void primitiveCameraCommandsCrossNativeBoundary() {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        map.moveBy(1, 2);
        var animation = new AnimationOptions().durationMs(0);
        map.moveByAnimated(0, 0);
        map.moveByAnimated(0, 0, animation);
        map.scaleBy(1.1);
        map.scaleBy(1.0, new ScreenPoint(32, 32));
        map.scaleByAnimated(1.0);
        map.scaleByAnimated(1.0, animation);
        map.scaleByAnimated(1.0, new ScreenPoint(32, 32));
        map.scaleByAnimated(1.0, new ScreenPoint(32, 32), animation);
        map.rotateBy(new ScreenPoint(0, 0), new ScreenPoint(1, 1));
        map.rotateByAnimated(new ScreenPoint(0, 0), new ScreenPoint(1, 1));
        map.rotateByAnimated(new ScreenPoint(0, 0), new ScreenPoint(1, 1), animation);
        map.pitchBy(0);
        map.pitchByAnimated(0);
        map.pitchByAnimated(0, animation);
        map.cancelTransitions();
      }
    }
  }

  @Test
  void basicStyleAndRenderRequestsCrossNativeBoundary() {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}");
        assertFalse(map.styleSourceExists("geojson-source"));
        map.addGeoJsonSourceUrl("geojson-source", "https://example.com/data.geojson");
        assertTrue(map.styleSourceExists("geojson-source"));
        assertEquals(SourceType.GEOJSON, map.styleSourceType("geojson-source").orElseThrow());
        assertTrue(map.styleSourceType("missing-source").isEmpty());
        map.setGeoJsonSourceUrl("geojson-source", "https://example.com/updated.geojson");
        assertTrue(map.removeStyleSource("geojson-source"));
        assertFalse(map.removeStyleSource("geojson-source"));
        map.setStyleJson(
            "{\"version\":8,\"sources\":{},\"layers\":[{\"id\":\"background-layer\",\"type\":\"background\"}]}");
        assertTrue(map.styleLayerExists("background-layer"));
        assertEquals("background", map.styleLayerType("background-layer").orElseThrow());
        assertTrue(map.styleLayerType("missing-layer").isEmpty());
        assertTrue(map.removeStyleLayer("background-layer"));
        assertFalse(map.removeStyleLayer("background-layer"));
        map.setStyleUrl("https://example.com/style.json");
        map.requestRepaint();
        assertThrows(InvalidStateException.class, map::requestStillImage);
      }
    }
  }
}
