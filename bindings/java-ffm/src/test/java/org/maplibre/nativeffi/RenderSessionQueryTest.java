package org.maplibre.nativeffi;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertInstanceOf;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;

import java.util.EnumSet;
import java.util.List;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.internal.NativeTestSupport;

final class RenderSessionQueryTest {
  private static final String QUERY_STYLE_JSON =
      """
      {
        "version": 8,
        "name": "java-query-test",
        "sources": {
          "point": {
            "type": "geojson",
            "data": {
              "type": "FeatureCollection",
              "features": [
                {
                  "type": "Feature",
                  "id": "feature-1",
                  "geometry": {"type": "Point", "coordinates": [-122.4194, 37.7749]},
                  "properties": {"kind": "capital", "visible": true}
                }
              ]
            }
          }
        },
        "layers": [
          {"id": "background", "type": "background", "paint": {"background-color": "#d8f1ff"}},
          {"id": "point-circle", "type": "circle", "source": "point", "paint": {"circle-color": "#f97316", "circle-radius": 12}}
        ]
      }
      """;

  private static final String CLUSTER_STYLE_JSON =
      """
      {
        "version": 8,
        "name": "java-cluster-query-test",
        "sources": {
          "cluster-source": {
            "type": "geojson",
            "cluster": true,
            "data": {
              "type": "FeatureCollection",
              "features": [
                {"type":"Feature","geometry":{"type":"Point","coordinates":[0.0,0.0]},"properties":{"name":"one"}},
                {"type":"Feature","geometry":{"type":"Point","coordinates":[0.001,0.001]},"properties":{"name":"two"}},
                {"type":"Feature","geometry":{"type":"Point","coordinates":[0.002,0.002]},"properties":{"name":"three"}}
              ]
            }
          }
        },
        "layers": [
          {"id":"background","type":"background","paint":{"background-color":"#ffffff"}},
          {"id":"cluster-circle","type":"circle","source":"cluster-source","filter":["has","point_count"],"paint":{"circle-color":"#2563eb","circle-radius":20}}
        ]
      }
      """;

  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibrary();
  }

  @AfterEach
  void restoreProcessState() {
    MapLibre.clearLogCallback();
    MapLibre.restoreDefaultAsyncLogSeverities();
  }

  @Test
  void featureStateSetGetRemoveCopiesSnapshots() throws Exception {
    MapLibre.setLogCallback(record -> true);
    MapLibre.setAsyncLogSeverities(EnumSet.noneOf(LogSeverity.class));

    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().setSize(64, 64));
    var session = map.attachOwnedTexture(new OwnedTextureDescriptor().setSize(64, 64));
    try {
      var selector = new FeatureStateSelector("point").setFeatureId("feature-1");
      var state =
          JsonValue.object(
              List.of(
                  new JsonValue.Member("hover", JsonValue.of(true)),
                  new JsonValue.Member("radius", JsonValue.unsigned(20))));
      assertThrows(InvalidStateException.class, () -> session.setFeatureState(selector, state));
      assertThrows(
          IllegalStateException.class,
          () -> new FeatureStateSelector("point").setStateKey("hover"));

      loadStyleAndRender(runtime, map, session);
      assertThrows(
          InvalidArgumentException.class,
          () -> session.setFeatureState(selector, JsonValue.array(List.of())));
      session.setFeatureState(selector, state);
      var copied = assertInstanceOf(JsonValue.ObjectValue.class, session.getFeatureState(selector));
      assertEquals(JsonValue.of(true), member(copied, "hover"));
      assertEquals(JsonValue.unsigned(20), member(copied, "radius"));

      session.removeFeatureState(
          new FeatureStateSelector("point").setFeatureId("feature-1").setStateKey("hover"));
      renderIfAvailable(runtime, map, session);
      var afterRemove =
          assertInstanceOf(JsonValue.ObjectValue.class, session.getFeatureState(selector));
      assertEquals(JsonValue.unsigned(20), member(afterRemove, "radius"));
      assertFalse(hasMember(afterRemove, "hover"));
    } finally {
      session.close();
      map.close();
      runtime.close();
    }
  }

  @Test
  void renderedAndSourceQueriesCopyResults() throws Exception {
    MapLibre.setLogCallback(record -> true);
    MapLibre.setAsyncLogSeverities(EnumSet.noneOf(LogSeverity.class));

    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().setSize(64, 64));
    var session = map.attachOwnedTexture(new OwnedTextureDescriptor().setSize(64, 64));
    try {
      assertThrows(
          InvalidStateException.class,
          () ->
              session.queryRenderedFeatures(RenderedQueryGeometry.point(new ScreenPoint(32, 32))));

      loadStyleAndRender(runtime, map, session);
      var queryPoint = map.pixelForLatLng(new LatLng(37.7749, -122.4194));
      var geometry =
          RenderedQueryGeometry.box(
              new ScreenBox(
                  new ScreenPoint(queryPoint.x() - 20.0, queryPoint.y() - 20.0),
                  new ScreenPoint(queryPoint.x() + 20.0, queryPoint.y() + 20.0)));
      var filter =
          JsonValue.array(
              List.of(
                  JsonValue.of("=="),
                  JsonValue.array(List.of(JsonValue.of("get"), JsonValue.of("kind"))),
                  JsonValue.of("capital")));
      var rendered =
          session.queryRenderedFeatures(
              geometry,
              new RenderedFeatureQueryOptions()
                  .setLayerIds(List.of("point-circle"))
                  .setFilter(filter));
      assertEquals(1, rendered.size());
      assertEquals("point", rendered.getFirst().sourceId().orElseThrow());
      assertEquals(JsonValue.of("capital"), member(rendered.getFirst().feature(), "kind"));

      var source =
          session.querySourceFeatures("point", new SourceFeatureQueryOptions().setFilter(filter));
      assertEquals(1, source.size());
      assertEquals("point", source.getFirst().sourceId().orElseThrow());
      assertEquals(JsonValue.of("capital"), member(source.getFirst().feature(), "kind"));
    } finally {
      session.close();
      map.close();
      runtime.close();
    }
  }

  @Test
  void featureExtensionQueriesCopyValueAndFeatureCollectionResults() throws Exception {
    MapLibre.setLogCallback(record -> true);
    MapLibre.setAsyncLogSeverities(EnumSet.noneOf(LogSeverity.class));

    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().setSize(64, 64));
    var session = map.attachOwnedTexture(new OwnedTextureDescriptor().setSize(64, 64));
    try {
      loadClusterStyleAndRender(runtime, map, session);
      var queryPoint = map.pixelForLatLng(new LatLng(0.0, 0.0));
      var geometry =
          RenderedQueryGeometry.box(
              new ScreenBox(
                  new ScreenPoint(queryPoint.x() - 30.0, queryPoint.y() - 30.0),
                  new ScreenPoint(queryPoint.x() + 30.0, queryPoint.y() + 30.0)));
      var cluster =
          waitForRenderedCluster(
              runtime,
              map,
              session,
              geometry,
              new RenderedFeatureQueryOptions().setLayerIds(List.of("cluster-circle")));

      var children =
          assertInstanceOf(
              FeatureExtensionResult.FeatureCollection.class,
              session.queryFeatureExtension(
                  "cluster-source", cluster.feature(), "supercluster", "children"));
      assertTrue(children.features().size() > 0);

      var expansionZoom =
          assertInstanceOf(
              FeatureExtensionResult.Value.class,
              session.queryFeatureExtension(
                  "cluster-source", cluster.feature(), "supercluster", "expansion-zoom"));
      assertInstanceOf(JsonValue.UInt.class, expansionZoom.value());

      var leavesArguments =
          JsonValue.object(
              List.of(
                  new JsonValue.Member("limit", JsonValue.unsigned(1)),
                  new JsonValue.Member("offset", JsonValue.unsigned(0))));
      var leaves =
          assertInstanceOf(
              FeatureExtensionResult.FeatureCollection.class,
              session.queryFeatureExtension(
                  "cluster-source", cluster.feature(), "supercluster", "leaves", leavesArguments));
      assertEquals(1, leaves.features().size());
      assertThrows(
          InvalidArgumentException.class,
          () ->
              session.queryFeatureExtension(
                  "cluster-source",
                  cluster.feature(),
                  "supercluster",
                  "leaves",
                  JsonValue.array(List.of())));
    } finally {
      session.close();
      map.close();
      runtime.close();
    }
  }

  private static void loadStyleAndRender(
      RuntimeHandle runtime, MapHandle map, RenderSessionHandle session)
      throws InterruptedException {
    map.jumpTo(new CameraOptions().setCenter(37.7749, -122.4194).setZoom(10.0));
    map.setStyleJson(QUERY_STYLE_JSON);
    for (var index = 0; index < 5; index++) {
      renderIfAvailable(runtime, map, session);
    }
  }

  private static void loadClusterStyleAndRender(
      RuntimeHandle runtime, MapHandle map, RenderSessionHandle session)
      throws InterruptedException {
    map.jumpTo(new CameraOptions().setCenter(0.0, 0.0).setZoom(0.0));
    map.setStyleJson(CLUSTER_STYLE_JSON);
    for (var index = 0; index < 5; index++) {
      renderIfAvailable(runtime, map, session);
    }
  }

  private static QueriedFeature waitForRenderedCluster(
      RuntimeHandle runtime,
      MapHandle map,
      RenderSessionHandle session,
      RenderedQueryGeometry geometry,
      RenderedFeatureQueryOptions options)
      throws InterruptedException {
    for (var attempt = 0; attempt < 1000; attempt++) {
      var clusters = session.queryRenderedFeatures(geometry, options);
      if (clusters.size() == 1) {
        return clusters.getFirst();
      }
      renderPendingUpdates(runtime, map, session);
      Thread.sleep(1);
    }
    fail("Timed out waiting for rendered cluster");
    return null;
  }

  private static void renderIfAvailable(
      RuntimeHandle runtime, MapHandle map, RenderSessionHandle session)
      throws InterruptedException {
    waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE);
    try {
      session.renderUpdate();
    } catch (InvalidStateException ignored) {
      // Another update may arrive before the renderer has a fresh frame.
    }
  }

  private static void renderPendingUpdates(
      RuntimeHandle runtime, MapHandle map, RenderSessionHandle session) {
    runtime.runOnce();
    while (true) {
      var event = runtime.pollEvent();
      if (event.isEmpty()) {
        return;
      }
      var value = event.get();
      if (value.type() == RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE
          && value.mapSource().filter(source -> source == map).isPresent()) {
        try {
          session.renderUpdate();
        } catch (InvalidStateException ignored) {
          // A pending event may be stale after another update.
        }
      }
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

  private static JsonValue member(Feature feature, String key) {
    return feature.properties().stream()
        .filter(member -> member.key().equals(key))
        .map(JsonValue.Member::value)
        .findFirst()
        .orElseThrow();
  }

  private static JsonValue member(JsonValue.ObjectValue object, String key) {
    return object.members().stream()
        .filter(member -> member.key().equals(key))
        .map(JsonValue.Member::value)
        .findFirst()
        .orElseThrow();
  }

  private static boolean hasMember(JsonValue.ObjectValue object, String key) {
    return object.members().stream().anyMatch(member -> member.key().equals(key));
  }
}
