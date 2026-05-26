package org.maplibre.nativejni.map;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertSame;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.EnumSet;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.camera.AnimationOptions;
import org.maplibre.nativejni.camera.BoundOptions;
import org.maplibre.nativejni.camera.CameraFitOptions;
import org.maplibre.nativejni.camera.CameraOptions;
import org.maplibre.nativejni.camera.EdgeInsets;
import org.maplibre.nativejni.camera.FreeCameraOptions;
import org.maplibre.nativejni.error.InvalidArgumentException;
import org.maplibre.nativejni.error.InvalidStateException;
import org.maplibre.nativejni.geo.CanonicalTileId;
import org.maplibre.nativejni.geo.Feature;
import org.maplibre.nativejni.geo.GeoJson;
import org.maplibre.nativejni.geo.Geometry;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.geo.LatLngBounds;
import org.maplibre.nativejni.geo.Quaternion;
import org.maplibre.nativejni.geo.ScreenPoint;
import org.maplibre.nativejni.geo.Vec3;
import org.maplibre.nativejni.json.JsonValue;
import org.maplibre.nativejni.render.PremultipliedRgba8Image;
import org.maplibre.nativejni.runtime.RuntimeHandle;
import org.maplibre.nativejni.runtime.RuntimeOptions;
import org.maplibre.nativejni.style.CustomGeometrySourceOptions;
import org.maplibre.nativejni.style.LocationIndicatorImageKind;
import org.maplibre.nativejni.style.RasterDemEncoding;
import org.maplibre.nativejni.style.SourceInfo;
import org.maplibre.nativejni.style.SourceType;
import org.maplibre.nativejni.style.StyleImageOptions;
import org.maplibre.nativejni.style.TileScheme;
import org.maplibre.nativejni.style.TileSourceOptions;
import org.maplibre.nativejni.style.VectorTileEncoding;
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
      assertTrue(map.nativeAddress() != 0);

      map.close();
      assertTrue(map.isClosed());
      map.close();
      assertThrows(InvalidStateException.class, map::nativeAddress);
    }
  }

  @Test
  void invalidDimensionsReportSpecificJniDiagnostic() {
    assertThrows(
        InvalidArgumentException.class,
        () -> RuntimeHandle.create(new RuntimeOptions().assetPath("asset\0path")));

    try (var runtime = RuntimeHandle.create()) {
      var error =
          assertThrows(
              InvalidArgumentException.class,
              () -> MapHandle.create(runtime, new MapOptions().size(-1, 1)));
      assertTrue(error.diagnostic().contains("width and height"));
    }
  }

  @Test
  void sourceOptionsRejectNegativeIntegralOptionsBeforeCast() {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        assertThrows(
            InvalidArgumentException.class,
            () ->
                map.addVectorSourceUrl(
                    "negative-tile-size",
                    "https://example.com/vector.json",
                    new TileSourceOptions().tileSize(-1)));
        assertThrows(
            InvalidArgumentException.class,
            () ->
                map.addCustomGeometrySource(
                    "negative-buffer", new CustomGeometrySourceOptions(tileId -> {}).buffer(-1)));
        assertThrows(
            InvalidArgumentException.class,
            () ->
                map.addCustomGeometrySource(
                    "negative-custom-tile-size",
                    new CustomGeometrySourceOptions(tileId -> {}).tileSize(-1)));
      }
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
  void viewportAndTileOptionsCrossNativeBoundary() {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        map.setViewportOptions(
            new ViewportOptions()
                .northOrientation(NorthOrientation.RIGHT)
                .constrainMode(ConstrainMode.HEIGHT_ONLY)
                .viewportMode(ViewportMode.FLIPPED_Y)
                .frustumOffset(new EdgeInsets(1, 2, 3, 4)));

        var viewport = map.viewportOptions();
        assertTrue(viewport.hasNorthOrientation());
        assertEquals(NorthOrientation.RIGHT, viewport.northOrientation());
        assertTrue(viewport.hasConstrainMode());
        assertEquals(ConstrainMode.HEIGHT_ONLY, viewport.constrainMode());
        assertTrue(viewport.hasViewportMode());
        assertEquals(ViewportMode.FLIPPED_Y, viewport.viewportMode());
        assertTrue(viewport.hasFrustumOffset());
        assertEquals(1, viewport.frustumOffset().top(), 1.0e-9);
        assertEquals(2, viewport.frustumOffset().left(), 1.0e-9);
        assertEquals(3, viewport.frustumOffset().bottom(), 1.0e-9);
        assertEquals(4, viewport.frustumOffset().right(), 1.0e-9);

        map.setTileOptions(
            new TileOptions()
                .prefetchZoomDelta(2)
                .lodMinRadius(1.5)
                .lodScale(2.5)
                .lodPitchThreshold(35.0)
                .lodZoomShift(0.75)
                .lodMode(TileLodMode.DISTANCE));

        var tile = map.tileOptions();
        assertTrue(tile.hasPrefetchZoomDelta());
        assertEquals(2, tile.prefetchZoomDelta());
        assertTrue(tile.hasLodMinRadius());
        assertEquals(1.5, tile.lodMinRadius(), 1.0e-9);
        assertTrue(tile.hasLodScale());
        assertEquals(2.5, tile.lodScale(), 1.0e-9);
        assertTrue(tile.hasLodPitchThreshold());
        assertEquals(35.0, tile.lodPitchThreshold(), 1.0e-9);
        assertTrue(tile.hasLodZoomShift());
        assertEquals(0.75, tile.lodZoomShift(), 1.0e-9);
        assertTrue(tile.hasLodMode());
        assertEquals(TileLodMode.DISTANCE, tile.lodMode());
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

        var geometryCamera =
            map.cameraForGeometry(
                Geometry.lineString(List.of(bounds.southwest(), bounds.northeast())), fit);
        assertTrue(geometryCamera.hasCenter());
        assertTrue(geometryCamera.hasZoom());

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
        var styleImage = new PremultipliedRgba8Image(1, 1, 4, new byte[] {1, 2, 3, 4});
        map.setStyleImage(
            "style-image", styleImage, new StyleImageOptions().pixelRatio(2.0f).sdf(true));
        assertTrue(map.styleImageExists("style-image"));
        var styleImageInfo = map.styleImageInfo("style-image").orElseThrow();
        assertEquals(1, styleImageInfo.width());
        assertEquals(1, styleImageInfo.height());
        assertEquals(4, styleImageInfo.stride());
        assertEquals(4, styleImageInfo.byteLength());
        assertEquals(2.0f, styleImageInfo.pixelRatio());
        assertTrue(styleImageInfo.sdf());
        var copiedStyleImage = map.copyStyleImagePremultipliedRgba8("style-image").orElseThrow();
        assertEquals(styleImage, copiedStyleImage.image());
        assertEquals(2.0f, copiedStyleImage.pixelRatio());
        assertTrue(copiedStyleImage.sdf());
        assertTrue(map.styleImageInfo("missing-style-image").isEmpty());
        assertTrue(map.removeStyleImage("style-image"));
        assertFalse(map.removeStyleImage("style-image"));
        assertFalse(map.styleImageExists("style-image"));
        assertFalse(map.copyStyleImagePremultipliedRgba8("style-image").isPresent());
        assertFalse(map.styleSourceExists("geojson-source"));
        map.addGeoJsonSourceUrl("geojson-source", "https://example.com/data.geojson");
        assertTrue(map.styleSourceExists("geojson-source"));
        assertEquals(SourceType.GEOJSON, map.styleSourceType("geojson-source").orElseThrow());
        assertTrue(map.styleSourceType("missing-source").isEmpty());
        map.setGeoJsonSourceUrl("geojson-source", "https://example.com/updated.geojson");
        assertTrue(map.removeStyleSource("geojson-source"));
        assertFalse(map.removeStyleSource("geojson-source"));
        map.addGeoJsonSourceData(
            "geojson-data-source",
            GeoJson.featureCollection(
                List.of(
                    new Feature(
                        Geometry.point(new LatLng(0.25, 0.5)),
                        List.of(new JsonValue.Member("name", JsonValue.of("one")))))));
        assertEquals(SourceType.GEOJSON, map.styleSourceType("geojson-data-source").orElseThrow());
        map.setGeoJsonSourceData(
            "geojson-data-source",
            GeoJson.geometry(Geometry.lineString(List.of(new LatLng(0, 0), new LatLng(1, 1)))));
        assertTrue(map.removeStyleSource("geojson-data-source"));
        map.addStyleSourceJson(
            "json-geojson-source",
            JsonValue.object(
                List.of(
                    new JsonValue.Member("type", JsonValue.of("geojson")),
                    new JsonValue.Member(
                        "data",
                        JsonValue.object(
                            List.of(
                                new JsonValue.Member("type", JsonValue.of("FeatureCollection")),
                                new JsonValue.Member("features", JsonValue.array(List.of()))))))));
        assertEquals(SourceType.GEOJSON, map.styleSourceType("json-geojson-source").orElseThrow());
        assertTrue(map.removeStyleSource("json-geojson-source"));

        var customFetchCount = new AtomicInteger();
        map.addCustomGeometrySource(
            "custom-source",
            new CustomGeometrySourceOptions(tileId -> customFetchCount.incrementAndGet())
                .minZoom(1.0)
                .maxZoom(12.0)
                .tileSize(512)
                .buffer(64)
                .clip(true)
                .wrap(false));
        assertEquals(1, map.customGeometrySourceCountForTesting());
        assertEquals(SourceType.CUSTOM_VECTOR, map.styleSourceType("custom-source").orElseThrow());
        var customTile = new CanonicalTileId(0, 0, 0);
        map.setCustomGeometrySourceTileData(
            "custom-source", customTile, GeoJson.featureCollection(List.of()));
        map.invalidateCustomGeometrySourceTile("custom-source", customTile);
        var invalidZoomTile = new CanonicalTileId(-1, 0, 0);
        assertThrows(
            InvalidArgumentException.class,
            () ->
                map.setCustomGeometrySourceTileData(
                    "custom-source", invalidZoomTile, GeoJson.featureCollection(List.of())));
        assertThrows(
            InvalidArgumentException.class,
            () -> map.invalidateCustomGeometrySourceTile("custom-source", invalidZoomTile));
        map.invalidateCustomGeometrySourceRegion(
            "custom-source", new LatLngBounds(new LatLng(-1.0, -2.0), new LatLng(1.0, 2.0)));
        assertTrue(map.removeStyleSource("custom-source"));
        assertEquals(0, map.customGeometrySourceCountForTesting());

        map.addVectorSourceUrl(
            "vector-source",
            "https://example.com/vector.json",
            new TileSourceOptions()
                .minZoom(1.0)
                .maxZoom(12.0)
                .vectorEncoding(VectorTileEncoding.MVT));
        assertTrue(map.styleSourceIds().contains("vector-source"));
        assertEquals(SourceType.VECTOR, map.styleSourceType("vector-source").orElseThrow());
        assertTrue(map.removeStyleSource("vector-source"));
        map.addVectorSourceTiles(
            "vector-tiles-source", List.of("https://example.com/vector/{z}/{x}/{y}.pbf"));
        assertEquals(SourceType.VECTOR, map.styleSourceType("vector-tiles-source").orElseThrow());
        assertTrue(map.removeStyleSource("vector-tiles-source"));
        map.addRasterSourceUrl(
            "raster-source",
            "https://example.com/raster.json",
            new TileSourceOptions()
                .attribution("© raster")
                .scheme(TileScheme.XYZ)
                .tileSize(256)
                .bounds(new LatLngBounds(new LatLng(-1.0, -2.0), new LatLng(1.0, 2.0))));
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
        map.addRasterDemSourceUrl(
            "raster-dem-source",
            "https://example.com/raster-dem.json",
            new TileSourceOptions().rasterDemEncoding(RasterDemEncoding.MAPBOX));
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
        map.addImageSourceImage("inline-image-source", imageCoordinates, styleImage);
        assertEquals(SourceType.IMAGE, map.styleSourceType("inline-image-source").orElseThrow());
        map.setImageSourceImage(
            "inline-image-source", new PremultipliedRgba8Image(1, 1, 4, new byte[] {5, 6, 7, 8}));
        assertTrue(map.removeStyleSource("inline-image-source"));
        map.setStyleJson(
            "{\"version\":8,\"sources\":{},\"layers\":[{\"id\":\"background-layer\",\"type\":\"background\"}]}");
        assertTrue(map.styleLayerExists("background-layer"));
        map.addStyleLayerJson(
            JsonValue.object(
                List.of(
                    new JsonValue.Member("id", JsonValue.of("json-background-layer")),
                    new JsonValue.Member("type", JsonValue.of("background")))));
        assertEquals("background", map.styleLayerType("json-background-layer").orElseThrow());
        assertTrue(map.styleLayerJson("json-background-layer").isPresent());
        assertTrue(map.styleLayerJson("missing-layer").isEmpty());
        map.setLayerProperty("json-background-layer", "background-color", JsonValue.of("#ff0000"));
        assertTrue(map.layerProperty("json-background-layer", "background-color").isPresent());
        map.addVectorSourceUrl("json-vector-source", "https://example.com/vector.json");
        map.addStyleLayerJson(
            JsonValue.object(
                List.of(
                    new JsonValue.Member("id", JsonValue.of("json-circle-layer")),
                    new JsonValue.Member("type", JsonValue.of("circle")),
                    new JsonValue.Member("source", JsonValue.of("json-vector-source")),
                    new JsonValue.Member("source-layer", JsonValue.of("pois")))));
        map.setLayerFilter(
            "json-circle-layer",
            JsonValue.array(
                List.of(
                    JsonValue.of("=="),
                    JsonValue.array(List.of(JsonValue.of("get"), JsonValue.of("kind"))),
                    JsonValue.of("park"))));
        assertTrue(map.layerFilter("json-circle-layer").isPresent());
        map.clearLayerFilter("json-circle-layer");
        map.setStyleLightJson(
            JsonValue.object(
                List.of(
                    new JsonValue.Member("anchor", JsonValue.of("viewport")),
                    new JsonValue.Member("color", JsonValue.of("white")),
                    new JsonValue.Member("intensity", JsonValue.of(0.4)))));
        map.setStyleLightProperty("intensity", JsonValue.of(0.6));
        assertTrue(map.styleLightProperty("intensity").isPresent());
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
