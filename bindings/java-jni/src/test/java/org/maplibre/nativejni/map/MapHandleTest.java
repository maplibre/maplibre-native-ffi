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
import org.maplibre.nativejni.camera.BoundOptions;
import org.maplibre.nativejni.camera.CameraFitOptions;
import org.maplibre.nativejni.camera.CameraOptions;
import org.maplibre.nativejni.camera.EdgeInsets;
import org.maplibre.nativejni.camera.FreeCameraOptions;
import org.maplibre.nativejni.error.InvalidStateException;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.geo.LatLngBounds;
import org.maplibre.nativejni.geo.Quaternion;
import org.maplibre.nativejni.geo.ScreenPoint;
import org.maplibre.nativejni.geo.Vec3;
import org.maplibre.nativejni.internal.access.InternalAccess;
import org.maplibre.nativejni.runtime.RuntimeHandle;
import org.maplibre.nativejni.style.LocationIndicatorImageKind;
import org.maplibre.nativejni.style.SourceInfo;
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
  void freeCameraCrossNativeBoundary() {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        map.setFreeCameraOptions(
            new FreeCameraOptions()
                .position(new Vec3(0.1, 0.2, 0.3))
                .orientation(new Quaternion(0, 0, 0, 1)));

        var camera = map.freeCameraOptions();
        assertTrue(camera.hasPosition());
        assertTrue(Double.isFinite(camera.position().x()));
        assertTrue(Double.isFinite(camera.position().y()));
        assertTrue(Double.isFinite(camera.position().z()));
        assertTrue(camera.hasOrientation());
      }
    }
  }

  @Test
  void projectionModeCrossNativeBoundary() {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        map.setProjectionMode(new ProjectionModeOptions().axonometric(true).xSkew(0.25).ySkew(0.5));

        var mode = map.projectionMode();
        assertTrue(mode.hasAxonometric());
        assertTrue(mode.axonometric());
        assertTrue(mode.hasXSkew());
        assertEquals(0.25, mode.xSkew(), 1.0e-9);
        assertTrue(mode.hasYSkew());
        assertEquals(0.5, mode.ySkew(), 1.0e-9);
      }
    }
  }

  @Test
  void cameraBoundsCrossNativeBoundary() {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        var bounds = new LatLngBounds(new LatLng(-10, -20), new LatLng(10, 20));
        map.setBounds(
            new BoundOptions().bounds(bounds).minZoom(1).maxZoom(10).minPitch(0).maxPitch(60));

        var result = map.bounds();
        assertTrue(result.hasBounds());
        assertEquals(-10, result.bounds().southwest().latitude(), 1.0e-9);
        assertEquals(-20, result.bounds().southwest().longitude(), 1.0e-9);
        assertEquals(10, result.bounds().northeast().latitude(), 1.0e-9);
        assertEquals(20, result.bounds().northeast().longitude(), 1.0e-9);
        assertEquals(1, result.minZoom(), 1.0e-9);
        assertEquals(10, result.maxZoom(), 1.0e-9);
        assertEquals(0, result.minPitch(), 1.0e-9);
        assertEquals(60, result.maxPitch(), 1.0e-9);
      }
    }
  }

  @Test
  void cameraFitQueriesCrossNativeBoundary() {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(256, 256))) {
        var bounds = new LatLngBounds(new LatLng(-1, -1), new LatLng(1, 1));
        var fit = new CameraFitOptions().padding(new EdgeInsets(4, 4, 4, 4)).bearing(0).pitch(0);

        var boundsCamera = map.cameraForLatLngBounds(bounds, fit);
        assertTrue(boundsCamera.hasCenter());
        assertTrue(boundsCamera.hasZoom());

        var coordinatesCamera =
            map.cameraForLatLngs(List.of(bounds.southwest(), bounds.northeast()));
        assertTrue(coordinatesCamera.hasCenter());
        assertTrue(coordinatesCamera.hasZoom());

        var visibleBounds = map.latLngBoundsForCamera(new CameraOptions().center(0, 0).zoom(1));
        assertTrue(Double.isFinite(visibleBounds.southwest().latitude()));
        assertTrue(Double.isFinite(visibleBounds.northeast().longitude()));

        var unwrappedBounds =
            map.latLngBoundsForCameraUnwrapped(new CameraOptions().center(0, 0).zoom(1));
        assertTrue(Double.isFinite(unwrappedBounds.southwest().latitude()));
        assertTrue(Double.isFinite(unwrappedBounds.northeast().longitude()));
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

        map.addVectorSourceUrl("vector-source", "https://example.com/vector.json");
        assertTrue(map.styleSourceIds().contains("vector-source"));
        assertEquals(SourceType.VECTOR, map.styleSourceType("vector-source").orElseThrow());
        assertTrue(map.removeStyleSource("vector-source"));
        map.addVectorSourceTiles(
            "vector-tiles-source", List.of("https://example.com/vector/{z}/{x}/{y}.pbf"));
        assertEquals(SourceType.VECTOR, map.styleSourceType("vector-tiles-source").orElseThrow());
        assertTrue(map.removeStyleSource("vector-tiles-source"));
        map.addRasterSourceUrl("raster-source", "https://example.com/raster.json");
        assertEquals(SourceType.RASTER, map.styleSourceType("raster-source").orElseThrow());
        SourceInfo rasterInfo = map.styleSourceInfo("raster-source").orElseThrow();
        assertEquals(SourceType.RASTER, rasterInfo.type());
        assertEquals(SourceType.RASTER.nativeValue(), rasterInfo.nativeType());
        assertFalse(rasterInfo.volatileSource());
        assertTrue(rasterInfo.attribution().isEmpty());
        assertTrue(map.styleSourceInfo("missing-source").isEmpty());
        assertTrue(map.removeStyleSource("raster-source"));
        map.addRasterSourceTiles(
            "raster-tiles-source", List.of("https://example.com/raster/{z}/{x}/{y}.png"));
        assertEquals(SourceType.RASTER, map.styleSourceType("raster-tiles-source").orElseThrow());
        assertTrue(map.removeStyleSource("raster-tiles-source"));
        map.addRasterDemSourceUrl("raster-dem-source", "https://example.com/raster-dem.json");
        assertEquals(SourceType.RASTER_DEM, map.styleSourceType("raster-dem-source").orElseThrow());
        assertTrue(map.removeStyleSource("raster-dem-source"));
        map.addRasterDemSourceTiles(
            "raster-dem-tiles-source", List.of("https://example.com/dem/{z}/{x}/{y}.png"));
        assertEquals(
            SourceType.RASTER_DEM, map.styleSourceType("raster-dem-tiles-source").orElseThrow());
        assertTrue(map.removeStyleSource("raster-dem-tiles-source"));
        List<LatLng> imageCoordinates =
            List.of(
                new LatLng(1.0, 2.0),
                new LatLng(1.0, 3.0),
                new LatLng(0.0, 3.0),
                new LatLng(0.0, 2.0));
        map.addImageSourceUrl("image-source", imageCoordinates, "https://example.com/image.png");
        assertEquals(SourceType.IMAGE, map.styleSourceType("image-source").orElseThrow());
        assertEquals(imageCoordinates, map.imageSourceCoordinates("image-source").orElseThrow());
        map.setImageSourceUrl("image-source", "https://example.com/updated-image.png");
        List<LatLng> updatedImageCoordinates =
            List.of(
                new LatLng(2.0, 4.0),
                new LatLng(2.0, 5.0),
                new LatLng(1.0, 5.0),
                new LatLng(1.0, 4.0));
        map.setImageSourceCoordinates("image-source", updatedImageCoordinates);
        assertEquals(
            updatedImageCoordinates, map.imageSourceCoordinates("image-source").orElseThrow());
        assertTrue(map.imageSourceCoordinates("missing-image-source").isEmpty());
        assertTrue(map.removeStyleSource("image-source"));
        map.setStyleJson(
            "{\"version\":8,\"sources\":{},\"layers\":[{\"id\":\"background-layer\",\"type\":\"background\"}]}");
        assertTrue(map.styleLayerExists("background-layer"));
        assertTrue(map.styleLayerIds().contains("background-layer"));
        assertEquals("background", map.styleLayerType("background-layer").orElseThrow());
        map.addRasterDemSourceUrl("dem-source", "https://example.com/dem.json");
        map.addHillshadeLayer("hillshade-layer", "dem-source");
        assertEquals("hillshade", map.styleLayerType("hillshade-layer").orElseThrow());
        map.addColorReliefLayer("relief-layer", "dem-source", "hillshade-layer");
        assertEquals("color-relief", map.styleLayerType("relief-layer").orElseThrow());
        map.addLocationIndicatorLayer("location-layer");
        assertEquals("location-indicator", map.styleLayerType("location-layer").orElseThrow());
        map.setLocationIndicatorLocation("location-layer", new LatLng(1.0, 2.0), 3.0);
        map.setLocationIndicatorBearing("location-layer", 45.0);
        map.setLocationIndicatorAccuracyRadius("location-layer", 12.0);
        map.setLocationIndicatorImageName(
            "location-layer", LocationIndicatorImageKind.TOP, "location-top-image");
        map.setStyleJson(
            "{\"version\":8,\"sources\":{},\"layers\":[{\"id\":\"first-layer\",\"type\":\"background\"},{\"id\":\"second-layer\",\"type\":\"background\"}]}");
        map.moveStyleLayer("second-layer", "first-layer");
        List<String> movedLayerIds = map.styleLayerIds();
        assertTrue(movedLayerIds.indexOf("second-layer") < movedLayerIds.indexOf("first-layer"));
        map.moveStyleLayer("second-layer");
        assertTrue(map.styleLayerIds().contains("second-layer"));
        assertTrue(map.styleLayerType("missing-layer").isEmpty());
        assertTrue(map.removeStyleLayer("first-layer"));
        assertFalse(map.removeStyleLayer("first-layer"));
        map.setStyleUrl("https://example.com/style.json");
        map.requestRepaint();
        assertThrows(InvalidStateException.class, map::requestStillImage);
      }
    }
  }
}
