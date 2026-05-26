package org.maplibre.nativejni.render;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertInstanceOf;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.lang.reflect.Constructor;
import java.util.List;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.error.MaplibreException;
import org.maplibre.nativejni.geo.Feature;
import org.maplibre.nativejni.geo.Geometry;
import org.maplibre.nativejni.geo.ScreenPoint;
import org.maplibre.nativejni.json.JsonValue;
import org.maplibre.nativejni.map.MapHandle;
import org.maplibre.nativejni.map.MapOptions;
import org.maplibre.nativejni.query.RenderedFeatureQueryOptions;
import org.maplibre.nativejni.query.RenderedQueryGeometry;
import org.maplibre.nativejni.query.SourceFeatureQueryOptions;
import org.maplibre.nativejni.runtime.RuntimeHandle;
import org.maplibre.nativejni.test.NativeTestSupport;

final class RenderSessionQueryTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibraryOrSkip();
  }

  @Test
  void queryOptionValuesCopyCallerLists() {
    var layers = new java.util.ArrayList<>(List.of("water"));
    var rendered = new RenderedFeatureQueryOptions().layerIds(layers).filter(JsonValue.of(true));
    layers.add("road");

    assertEquals(List.of("water"), rendered.layerIds());
    assertTrue(rendered.hasFilter());

    var sourceLayers = new java.util.ArrayList<>(List.of("landuse"));
    var source = new SourceFeatureQueryOptions().sourceLayerIds(sourceLayers);
    sourceLayers.add("building");
    assertEquals(List.of("landuse"), source.sourceLayerIds());
  }

  @Test
  void renderedQueryGeometryWrapsPointGeometry() {
    var geometry = RenderedQueryGeometry.point(new ScreenPoint(1, 2));

    var point = assertInstanceOf(RenderedQueryGeometry.Point.class, geometry).point();
    assertEquals(1, point.x(), 1.0e-9);
    assertEquals(2, point.y(), 1.0e-9);
  }

  @Test
  void publicQueryMethodsCrossJavaCppBoundary() throws Exception {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        var session = invalidLiveSessionWrapper(map);
        assertThrows(
            MaplibreException.class,
            () ->
                session.queryRenderedFeatures(RenderedQueryGeometry.point(new ScreenPoint(1, 2))));
        assertThrows(MaplibreException.class, () -> session.querySourceFeatures("source"));
        assertThrows(
            MaplibreException.class,
            () ->
                session.queryFeatureExtension(
                    "source", new Feature(Geometry.empty(), List.of()), "ext", "field"));
      }
    }
  }

  private static RenderSessionHandle invalidLiveSessionWrapper(MapHandle map) throws Exception {
    Constructor<RenderSessionHandle> constructor =
        RenderSessionHandle.class.getDeclaredConstructor(MapHandle.class, long.class);
    constructor.setAccessible(true);
    return constructor.newInstance(map, 1L);
  }
}
