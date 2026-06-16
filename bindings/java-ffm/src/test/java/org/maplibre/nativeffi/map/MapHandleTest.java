package org.maplibre.nativeffi.map;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.EnumSet;
import java.util.List;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.camera.BoundOptions;
import org.maplibre.nativeffi.camera.CameraFitOptions;
import org.maplibre.nativeffi.camera.CameraOptions;
import org.maplibre.nativeffi.camera.EdgeInsets;
import org.maplibre.nativeffi.error.InvalidArgumentException;
import org.maplibre.nativeffi.error.InvalidStateException;
import org.maplibre.nativeffi.error.MaplibreStatus;
import org.maplibre.nativeffi.error.WrongThreadException;
import org.maplibre.nativeffi.geo.Geometry;
import org.maplibre.nativeffi.geo.LatLng;
import org.maplibre.nativeffi.geo.LatLngBounds;
import org.maplibre.nativeffi.geo.ScreenPoint;
import org.maplibre.nativeffi.internal.convert.NativeValues;
import org.maplibre.nativeffi.runtime.RuntimeHandle;
import org.maplibre.nativeffi.runtime.RuntimeOptions;
import org.maplibre.nativeffi.test.NativeTestSupport;

final class MapHandleTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibrary();
  }

  @Test
  void unknownMapOptionEnumsPreserveRawValues() {
    var northOrientation = NativeValues.northOrientation(999_991);
    var constrainMode = NativeValues.constrainMode(999_992);
    var viewportMode = NativeValues.viewportMode(999_993);
    var tileLodMode = NativeValues.tileLodMode(999_994);

    assertEquals(999_991, northOrientation.rawValue());
    assertEquals(999_992, constrainMode.rawValue());
    assertEquals(999_993, viewportMode.rawValue());
    assertEquals(999_994, tileLodMode.rawValue());
    assertThrows(InvalidArgumentException.class, () -> NativeValues.nativeValue(northOrientation));
    assertThrows(InvalidArgumentException.class, () -> NativeValues.nativeValue(constrainMode));
    assertThrows(InvalidArgumentException.class, () -> NativeValues.nativeValue(viewportMode));
    assertThrows(InvalidArgumentException.class, () -> NativeValues.nativeValue(tileLodMode));
  }

  @Test
  void createsAndClosesMapBeforeRuntime() {
    var runtime = RuntimeHandle.create(new RuntimeOptions());
    var map = MapHandle.create(runtime, new MapOptions().size(128, 128));
    try {
      map.requestRepaint();
    } finally {
      map.close();
      assertTrue(map.isClosed());
      runtime.close();
    }
  }

  @Test
  void mapOptionSemanticValidationComesFromNative() {
    var runtime = RuntimeHandle.create(new RuntimeOptions());
    try {
      var error =
          assertThrows(
              InvalidArgumentException.class,
              () -> MapHandle.create(runtime, new MapOptions().size(0, 128)));
      assertEquals(MaplibreStatus.INVALID_ARGUMENT, error.status());
      assertFalse(error.diagnostic().isBlank());
    } finally {
      runtime.close();
    }
  }

  @Test
  void runtimeCloseFailsWhileMapIsLive() {
    var runtime = RuntimeHandle.create(new RuntimeOptions());
    var map = MapHandle.create(runtime, new MapOptions().size(128, 128));
    try {
      var error = assertThrows(InvalidStateException.class, runtime::close);
      assertEquals(MaplibreStatus.INVALID_STATE, error.status());
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void releasedMapRejectsLaterMethodsBeforeNativeDispatch() {
    var runtime = RuntimeHandle.create(new RuntimeOptions());
    try {
      var map = MapHandle.create(runtime, new MapOptions().size(128, 128));
      map.close();
      var error = assertThrows(InvalidStateException.class, () -> map.setStyleJson("{}"));
      assertTrue(error.diagnostic().contains("MapHandle"));
      assertThrows(InvalidStateException.class, () -> map.cameraForLatLngs(List.of(), null));
      assertThrows(InvalidStateException.class, () -> map.pixelsForLatLngs(List.of()));
      assertThrows(InvalidStateException.class, () -> map.latLngsForPixels(List.of()));
    } finally {
      runtime.close();
    }
  }

  @Test
  void publicStyleStringInputsRejectEmbeddedNul() {
    var runtime = RuntimeHandle.create(new RuntimeOptions());
    var map = MapHandle.create(runtime, new MapOptions().size(128, 128));
    try {
      assertThrows(IllegalArgumentException.class, () -> map.setStyleUrl("custom://a\0b"));
      assertThrows(IllegalArgumentException.class, () -> map.setStyleJson("{\0}"));
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void debugAndStateHelpersRoundTrip() {
    var runtime = RuntimeHandle.create(new RuntimeOptions());
    var map = MapHandle.create(runtime, new MapOptions().size(128, 128));
    try {
      map.setDebugOptions(EnumSet.of(DebugOption.TILE_BORDERS, DebugOption.TIMESTAMPS));
      assertEquals(
          EnumSet.of(DebugOption.TILE_BORDERS, DebugOption.TIMESTAMPS), map.debugOptions());

      map.setRenderingStatsViewEnabled(true);
      assertTrue(map.isRenderingStatsViewEnabled());
      map.setRenderingStatsViewEnabled(false);
      assertFalse(map.isRenderingStatsViewEnabled());

      map.isFullyLoaded();
      map.dumpDebugLogs();
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void viewportTileBoundsAndProjectionModeOptionsRoundTrip() {
    var runtime = RuntimeHandle.create(new RuntimeOptions());
    var map = MapHandle.create(runtime, new MapOptions().size(128, 128));
    try {
      map.setViewportOptions(
          new ViewportOptions()
              .northOrientation(NorthOrientation.UP)
              .constrainMode(ConstrainMode.HEIGHT_ONLY)
              .viewportMode(ViewportMode.DEFAULT)
              .frustumOffset(EdgeInsets.ZERO));
      var viewport = map.viewportOptions();
      assertEquals(NorthOrientation.UP, viewport.northOrientation());
      assertEquals(ConstrainMode.HEIGHT_ONLY, viewport.constrainMode());
      assertEquals(ViewportMode.DEFAULT, viewport.viewportMode());
      assertEquals(EdgeInsets.ZERO, viewport.frustumOffset());
      map.setViewportOptions(viewport);

      map.setTileOptions(new TileOptions().prefetchZoomDelta(2).lodMode(TileLodMode.DEFAULT));
      var tile = map.tileOptions();
      assertEquals(2, tile.prefetchZoomDelta());
      assertEquals(TileLodMode.DEFAULT, tile.lodMode());
      map.setTileOptions(tile);

      map.setBounds(new BoundOptions().minZoom(0).maxZoom(22).minPitch(0).maxPitch(60));
      var bounds = map.bounds();
      assertEquals(0.0, bounds.minZoom(), 0.0);
      assertEquals(22.0, bounds.maxZoom(), 0.0);

      map.setProjectionMode(new ProjectionModeOptions().axonometric(false).xSkew(0).ySkew(0));
      var projectionMode = map.projectionMode();
      assertFalse(projectionMode.axonometric());
      assertEquals(0.0, projectionMode.xSkew(), 0.0);
      assertEquals(0.0, projectionMode.ySkew(), 0.0);
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void cameraCommandsAndCoordinateConversionsUseRealMap() {
    var runtime = RuntimeHandle.create(new RuntimeOptions());
    var map = MapHandle.create(runtime, new MapOptions().size(256, 256));
    try {
      map.jumpTo(new CameraOptions().center(new LatLng(37.7749, -122.4194)).zoom(4).pitch(20));
      var camera = map.camera();
      assertEquals(37.7749, camera.center().latitude(), 1e-6);
      assertEquals(-122.4194, camera.center().longitude(), 1e-6);
      assertEquals(4.0, camera.zoom(), 1e-6);
      assertEquals(20.0, camera.pitch(), 1e-6);

      map.moveBy(0, 0);
      map.scaleBy(1.0, new ScreenPoint(128, 128));
      map.rotateBy(new ScreenPoint(128, 128), new ScreenPoint(128, 128));
      map.pitchBy(0);
      map.cancelTransitions();

      var point = map.pixelForLatLng(new LatLng(0, 0));
      var coordinate = map.latLngForPixel(point);
      assertEquals(0.0, coordinate.latitude(), 1e-6);
      assertEquals(0.0, coordinate.longitude(), 1e-6);

      var points = map.pixelsForLatLngs(List.of(new LatLng(0, 0), new LatLng(1, 1)));
      assertEquals(2, points.size());
      var coordinates = map.latLngsForPixels(points);
      assertEquals(2, coordinates.size());

      var fitted =
          map.cameraForLatLngBounds(
              new LatLngBounds(new LatLng(-1, -1), new LatLng(1, 1)),
              new CameraFitOptions().padding(EdgeInsets.ZERO));
      assertTrue(fitted.hasCenter());
      var geometryFitted =
          map.cameraForGeometry(
              Geometry.multiPoint(List.of(new LatLng(-1, -1), new LatLng(1, 1))),
              new CameraFitOptions().padding(EdgeInsets.ZERO));
      assertTrue(geometryFitted.hasCenter());
      map.latLngBoundsForCamera(fitted);
      map.latLngBoundsForCameraUnwrapped(fitted);
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void emptyCameraFitCoordinateInputsUseNativeValidation() {
    var runtime = RuntimeHandle.create(new RuntimeOptions());
    var map = MapHandle.create(runtime, new MapOptions().size(128, 128));
    var projection = map.createProjection();
    try {
      var mapError =
          assertThrows(InvalidArgumentException.class, () -> map.cameraForLatLngs(List.of(), null));
      assertEquals(MaplibreStatus.INVALID_ARGUMENT, mapError.status());
      assertFalse(mapError.diagnostic().isBlank());

      var projectionError =
          assertThrows(
              InvalidArgumentException.class,
              () -> projection.setVisibleCoordinates(List.of(), EdgeInsets.ZERO));
      assertEquals(MaplibreStatus.INVALID_ARGUMENT, projectionError.status());
      assertFalse(projectionError.diagnostic().isBlank());
    } finally {
      projection.close();
      map.close();
      runtime.close();
    }
  }

  @Test
  void projectionHelpersCloseIndependently() {
    var runtime = RuntimeHandle.create(new RuntimeOptions());
    var map = MapHandle.create(runtime, new MapOptions().size(128, 128));
    try {
      var projection = map.createProjection();
      projection.setCamera(new CameraOptions().center(new LatLng(0, 0)).zoom(2));
      var camera = projection.camera();
      assertEquals(0.0, camera.center().latitude(), 1e-6);
      assertEquals(0.0, camera.center().longitude(), 1e-6);
      assertEquals(2.0, camera.zoom(), 1e-6);
      projection.setVisibleCoordinates(
          List.of(new LatLng(-1, -1), new LatLng(1, 1)), EdgeInsets.ZERO);
      projection.setVisibleGeometry(
          Geometry.multiPoint(List.of(new LatLng(-1, -1), new LatLng(1, 1))), EdgeInsets.ZERO);
      var point = projection.pixelForLatLng(new LatLng(0, 0));
      var coordinate = projection.latLngForPixel(point);
      assertEquals(0.0, coordinate.latitude(), 1e-6);
      assertEquals(0.0, coordinate.longitude(), 1e-6);
      projection.close();
      assertThrows(InvalidStateException.class, () -> projection.pixelForLatLng(new LatLng(0, 0)));
      assertThrows(
          InvalidStateException.class,
          () -> projection.setVisibleCoordinates(List.of(), EdgeInsets.ZERO));
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void projectionSnapshotRemainsUsableAfterSourceMapCloses() {
    var runtime = RuntimeHandle.create(new RuntimeOptions());
    var map = MapHandle.create(runtime, new MapOptions().size(128, 128));
    var projection = map.createProjection();
    try {
      projection.setCamera(new CameraOptions().center(new LatLng(0, 0)).zoom(2));
      map.close();

      var point = projection.pixelForLatLng(new LatLng(0, 0));
      var coordinate = projection.latLngForPixel(point);
      assertEquals(0.0, coordinate.latitude(), 1e-6);
      assertEquals(0.0, coordinate.longitude(), 1e-6);
    } finally {
      projection.close();
      runtime.close();
    }
  }

  @Test
  void wrongThreadMapCallMapsToWrongThreadException() throws Exception {
    var runtime = RuntimeHandle.create(new RuntimeOptions());
    var map = MapHandle.create(runtime, new MapOptions().size(128, 128));
    try {
      assertWrongThread(runOnOtherThread(map::requestRepaint));
      assertWrongThread(runOnOtherThread(() -> map.cameraForLatLngs(List.of(), null)));
      assertWrongThread(runOnOtherThread(() -> map.pixelsForLatLngs(List.of())));
      assertWrongThread(runOnOtherThread(() -> map.latLngsForPixels(List.of())));
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void wrongThreadProjectionCallMapsToWrongThreadException() throws Exception {
    var runtime = RuntimeHandle.create(new RuntimeOptions());
    var map = MapHandle.create(runtime, new MapOptions().size(128, 128));
    var projection = map.createProjection();
    try {
      assertWrongThread(runOnOtherThread(() -> projection.pixelForLatLng(new LatLng(0, 0))));
      assertWrongThread(
          runOnOtherThread(() -> projection.setVisibleCoordinates(List.of(), EdgeInsets.ZERO)));
    } finally {
      projection.close();
      map.close();
      runtime.close();
    }
  }

  @Test
  void wrongThreadCloseLeavesMapAndProjectionLive() throws Exception {
    var runtime = RuntimeHandle.create(new RuntimeOptions());
    var map = MapHandle.create(runtime, new MapOptions().size(128, 128));
    var projection = map.createProjection();
    try {
      assertWrongThread(runOnOtherThread(map::close));
      assertFalse(map.isClosed());
      assertWrongThread(runOnOtherThread(projection::close));
      assertFalse(projection.isClosed());
    } finally {
      projection.close();
      map.close();
      runtime.close();
    }
  }

  private static void assertWrongThread(Throwable thrown) {
    assertTrue(thrown instanceof WrongThreadException, () -> String.valueOf(thrown));
    var error = (WrongThreadException) thrown;
    assertEquals(MaplibreStatus.WRONG_THREAD, error.status());
    assertFalse(error.diagnostic().isBlank());
  }

  private static Throwable runOnOtherThread(ThrowingRunnable action) throws InterruptedException {
    var thrown = new AtomicReference<Throwable>();
    var thread =
        new Thread(
            () -> {
              try {
                action.run();
              } catch (Throwable error) {
                thrown.set(error);
              }
            });
    thread.start();
    thread.join();
    return thrown.get();
  }

  @FunctionalInterface
  private interface ThrowingRunnable {
    void run() throws Exception;
  }
}
