package org.maplibre.nativeffi.map;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.geo.CanonicalTileId;
import org.maplibre.nativeffi.geo.Feature;
import org.maplibre.nativeffi.geo.GeoJson;
import org.maplibre.nativeffi.geo.Geometry;
import org.maplibre.nativeffi.geo.LatLng;
import org.maplibre.nativeffi.geo.LatLngBounds;
import org.maplibre.nativeffi.internal.c.mln_custom_geometry_source_options;
import org.maplibre.nativeffi.internal.c.mln_custom_geometry_source_tile_callback;
import org.maplibre.nativeffi.internal.struct.StyleStructs;
import org.maplibre.nativeffi.json.JsonValue;
import org.maplibre.nativeffi.render.PremultipliedRgba8Image;
import org.maplibre.nativeffi.runtime.RuntimeEventType;
import org.maplibre.nativeffi.runtime.RuntimeHandle;
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions;
import org.maplibre.nativeffi.style.LocationIndicatorImageKind;
import org.maplibre.nativeffi.style.RasterDemEncoding;
import org.maplibre.nativeffi.style.SourceType;
import org.maplibre.nativeffi.style.StyleImageOptions;
import org.maplibre.nativeffi.style.TileScheme;
import org.maplibre.nativeffi.style.TileSourceOptions;
import org.maplibre.nativeffi.style.VectorTileEncoding;
import org.maplibre.nativeffi.test.NativeTestSupport;

final class StyleHandleTest {
  private static final String EMPTY_STYLE = "{\"version\":8,\"sources\":{},\"layers\":[]}";

  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibrary();
  }

  @Test
  void styleSourceAndLayerApisCopyIdsAndSnapshots() {
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(128, 128));
    try {
      map.setStyleJson(EMPTY_STYLE);
      map.addGeoJsonSourceData(
          "parks",
          GeoJson.featureCollection(
              List.of(
                  new Feature(
                      Geometry.point(new LatLng(0, 0)),
                      List.of(new JsonValue.Member("kind", JsonValue.of("park")))))));

      assertTrue(map.styleSourceExists("parks"));
      assertEquals(Optional.of(SourceType.GEOJSON), map.styleSourceType("parks"));
      assertEquals(SourceType.GEOJSON, map.styleSourceInfo("parks").orElseThrow().type());
      assertTrue(map.styleSourceIds().contains("parks"));

      var layerJson =
          JsonValue.object(
              List.of(
                  new JsonValue.Member("id", JsonValue.of("park-circles")),
                  new JsonValue.Member("type", JsonValue.of("circle")),
                  new JsonValue.Member("source", JsonValue.of("parks"))));
      map.addStyleLayerJson(layerJson);
      assertTrue(map.styleLayerExists("park-circles"));
      assertEquals(Optional.of("circle"), map.styleLayerType("park-circles"));
      assertTrue(map.styleLayerIds().contains("park-circles"));
      assertTrue(map.styleLayerJson("park-circles").isPresent());

      map.setLayerProperty("park-circles", "circle-radius", JsonValue.of(5.0));
      assertTrue(map.layerProperty("park-circles", "circle-radius").isPresent());
      map.setLayerFilter(
          "park-circles", JsonValue.array(List.of(JsonValue.of("has"), JsonValue.of("kind"))));
      assertTrue(map.layerFilter("park-circles").isPresent());
      map.clearLayerFilter("park-circles");

      assertTrue(map.removeStyleLayer("park-circles"));
      assertFalse(map.styleLayerExists("park-circles"));
      assertTrue(map.removeStyleSource("parks"));
      assertFalse(map.styleSourceExists("parks"));
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void tileSourceOptionsAndStyleImagesRoundTripThroughNativeMetadata() {
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(128, 128));
    try {
      map.setStyleJson(EMPTY_STYLE);
      map.addVectorSourceTiles(
          "vector",
          List.of("https://example.com/vector/{z}/{x}/{y}.pbf"),
          new TileSourceOptions()
              .minZoom(0)
              .maxZoom(14)
              .attribution("vector attribution")
              .scheme(TileScheme.XYZ)
              .tileSize(512)
              .vectorEncoding(VectorTileEncoding.MVT));
      assertEquals(Optional.of(SourceType.VECTOR), map.styleSourceType("vector"));
      assertEquals(
          Optional.of("vector attribution"),
          map.styleSourceInfo("vector").orElseThrow().attribution());

      map.addRasterSourceTiles(
          "raster",
          List.of("https://example.com/raster/{z}/{x}/{y}.png"),
          new TileSourceOptions().tileSize(256));
      assertEquals(Optional.of(SourceType.RASTER), map.styleSourceType("raster"));

      map.addRasterDemSourceTiles(
          "dem",
          List.of("https://example.com/dem/{z}/{x}/{y}.png"),
          new TileSourceOptions().tileSize(512).rasterDemEncoding(RasterDemEncoding.TERRARIUM));
      assertEquals(Optional.of(SourceType.RASTER_DEM), map.styleSourceType("dem"));

      assertThrows(
          IllegalArgumentException.class,
          () -> new PremultipliedRgba8Image(0, 1, 4, new byte[] {1, 2, 3, 4}));

      var image = new PremultipliedRgba8Image(1, 1, 4, new byte[] {1, 2, 3, 4});
      map.setStyleImage("dot", image, new StyleImageOptions().pixelRatio(2.0f).sdf(true));
      assertTrue(map.styleImageExists("dot"));
      var info = map.styleImageInfo("dot").orElseThrow();
      assertEquals(1, info.width());
      assertEquals(1, info.height());
      assertEquals(4, info.byteLength());
      assertEquals(2.0f, info.pixelRatio(), 0.0f);
      assertTrue(info.sdf());
      var copied = map.copyStyleImagePremultipliedRgba8("dot").orElseThrow();
      assertArrayEquals(image.pixels(), copied.image().pixels());
      assertEquals(2.0f, copied.pixelRatio(), 0.0f);
      assertTrue(copied.sdf());

      var padded =
          new PremultipliedRgba8Image(1, 2, 8, new byte[] {1, 2, 3, 4, 0, 0, 0, 0, 9, 10, 11, 12});
      map.setStyleImage("padded", padded);
      assertArrayEquals(
          new byte[] {1, 2, 3, 4, 9, 10, 11, 12},
          map.copyStyleImagePremultipliedRgba8("padded").orElseThrow().image().pixels());

      assertTrue(map.removeStyleImage("dot"));
      assertFalse(map.styleImageExists("dot"));
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void customGeometryCallbackStateOutlivesInFlightCallbacks() throws Exception {
    var entered = new CountDownLatch(1);
    var release = new CountDownLatch(1);
    var seenTile = new AtomicReference<CanonicalTileId>();
    var state =
        new CustomGeometrySourceState(
            new CustomGeometrySourceOptions(
                tileId -> {
                  seenTile.set(tileId);
                  entered.countDown();
                  try {
                    release.await(5, TimeUnit.SECONDS);
                  } catch (InterruptedException exception) {
                    Thread.currentThread().interrupt();
                  }
                }));
    var thread =
        new Thread(
            () -> {
              try (var arena = Arena.ofConfined()) {
                var tileId = StyleStructs.canonicalTileId(new CanonicalTileId(1, 1, 1), arena);
                mln_custom_geometry_source_tile_callback.invoke(
                    mln_custom_geometry_source_options.fetch_tile(state.descriptor()),
                    MemorySegment.NULL,
                    tileId);
              }
            });
    thread.start();

    assertTrue(entered.await(5, TimeUnit.SECONDS));
    state.close();
    mln_custom_geometry_source_options.cancel_tile(state.descriptor());
    release.countDown();
    thread.join(5000);

    assertFalse(thread.isAlive());
    assertEquals(new CanonicalTileId(1, 1, 1), seenTile.get());
    assertThrows(
        IllegalStateException.class,
        () -> mln_custom_geometry_source_options.fetch_tile(state.descriptor()));
  }

  @Test
  void imageSourcesAndLocationIndicatorHelpersUsePublicValues() throws Exception {
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(128, 128));
    try {
      map.setStyleJson(EMPTY_STYLE);
      drainEvents(runtime);
      var image = new PremultipliedRgba8Image(1, 1, 4, new byte[] {10, 20, 30, 40});
      var coordinates =
          List.of(new LatLng(1, 1), new LatLng(1, 2), new LatLng(0, 2), new LatLng(0, 1));
      map.addImageSourceImage("overlay", coordinates, image);
      assertEquals(Optional.of(SourceType.IMAGE), map.styleSourceType("overlay"));
      assertEquals(Optional.of(coordinates), map.imageSourceCoordinates("overlay"));

      var updatedCoordinates =
          List.of(new LatLng(2, 2), new LatLng(2, 3), new LatLng(1, 3), new LatLng(1, 2));
      map.setImageSourceCoordinates("overlay", updatedCoordinates);
      assertEquals(Optional.of(updatedCoordinates), map.imageSourceCoordinates("overlay"));

      map.addCustomGeometrySource(
          "custom",
          new CustomGeometrySourceOptions(tileId -> {})
              .minZoom(0)
              .maxZoom(2)
              .tileSize(512)
              .buffer(64)
              .clip(true)
              .wrap(false));
      assertEquals(Optional.of(SourceType.CUSTOM_VECTOR), map.styleSourceType("custom"));
      var tileId = new CanonicalTileId(0, 0, 0);
      map.setCustomGeometrySourceTileData("custom", tileId, GeoJson.featureCollection(List.of()));
      map.invalidateCustomGeometrySourceTile("custom", tileId);
      map.invalidateCustomGeometrySourceRegion(
          "custom", new LatLngBounds(new LatLng(-1, -1), new LatLng(1, 1)));
      assertTrue(map.removeStyleSource("custom"));

      map.addCustomGeometrySource("temporary-custom", new CustomGeometrySourceOptions(tile -> {}));
      assertEquals(1, map.customGeometrySourceCountForTesting());
      map.setStyleJson(EMPTY_STYLE);
      waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED);
      assertEquals(0, map.customGeometrySourceCountForTesting());

      map.addLocationIndicatorLayer("location");
      assertTrue(map.styleLayerExists("location"));
      assertEquals(Optional.of("location-indicator"), map.styleLayerType("location"));
      map.setLocationIndicatorLocation("location", new LatLng(37.7749, -122.4194), 15.0);
      map.setLocationIndicatorBearing("location", 45.0);
      map.setLocationIndicatorAccuracyRadius("location", 12.0);
      map.setStyleImage("location-top", image);
      map.setLocationIndicatorImageName("location", LocationIndicatorImageKind.TOP, "location-top");
    } finally {
      map.close();
      runtime.close();
    }
  }

  private static void drainEvents(RuntimeHandle runtime) {
    runtime.runOnce();
    while (runtime.pollEvent().isPresent()) {
      // Drain stale setup events so later style-loaded assertions observe the replacement under
      // test.
    }
  }

  private static void waitForMapEvent(RuntimeHandle runtime, MapHandle map, RuntimeEventType type)
      throws InterruptedException {
    for (var attempt = 0; attempt < 1000; attempt++) {
      runtime.runOnce();
      while (true) {
        var event = runtime.pollEvent();
        if (event.isEmpty()) {
          break;
        }
        var value = event.get();
        if (value.type() == type && value.mapSource().filter(source -> source == map).isPresent()) {
          return;
        }
      }
      Thread.sleep(1);
    }
    fail("Timed out waiting for " + type);
  }
}
