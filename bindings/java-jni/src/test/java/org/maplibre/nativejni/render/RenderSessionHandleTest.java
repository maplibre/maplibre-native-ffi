package org.maplibre.nativejni.render;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertInstanceOf;
import static org.junit.jupiter.api.Assertions.assertSame;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assertions.fail;

import java.util.List;
import java.util.Set;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.Maplibre;
import org.maplibre.nativejni.camera.CameraOptions;
import org.maplibre.nativejni.error.InvalidArgumentException;
import org.maplibre.nativejni.error.InvalidStateException;
import org.maplibre.nativejni.error.MaplibreException;
import org.maplibre.nativejni.error.MaplibreStatus;
import org.maplibre.nativejni.error.UnsupportedFeatureException;
import org.maplibre.nativejni.error.WrongThreadException;
import org.maplibre.nativejni.geo.Feature;
import org.maplibre.nativejni.geo.FeatureIdentifier;
import org.maplibre.nativejni.geo.Geometry;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.geo.ScreenBox;
import org.maplibre.nativejni.geo.ScreenPoint;
import org.maplibre.nativejni.internal.struct.QueryStructs;
import org.maplibre.nativejni.json.JsonValue;
import org.maplibre.nativejni.map.MapHandle;
import org.maplibre.nativejni.map.MapOptions;
import org.maplibre.nativejni.query.FeatureExtensionResult;
import org.maplibre.nativejni.query.FeatureStateSelector;
import org.maplibre.nativejni.query.QueriedFeature;
import org.maplibre.nativejni.query.RenderedFeatureQueryOptions;
import org.maplibre.nativejni.query.RenderedQueryGeometry;
import org.maplibre.nativejni.query.SourceFeatureQueryOptions;
import org.maplibre.nativejni.runtime.RuntimeEventType;
import org.maplibre.nativejni.runtime.RuntimeHandle;
import org.maplibre.nativejni.test.RenderTargetTestSupport;

class RenderSessionHandleTest {
  private static final double QUERY_COORDINATE_TOLERANCE = 1.0e-4;

  private static final String STYLE_JSON =
      """
      {
        "version": 8,
        "sources": {},
        "layers": [
          {"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}}
        ]
      }
      """;

  private static final String QUERY_STYLE_JSON =
      """
      {
        "version": 8,
        "name": "java-jni-query-test",
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
        "name": "java-jni-cluster-query-test",
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

  private static final String FEATURE_STATE_STYLE_JSON =
      """
      {
        "version": 8,
        "sources": {
          "point": {
            "type": "geojson",
            "data": {
              "type": "FeatureCollection",
              "features": [
                {
                  "type": "Feature",
                  "id": "feature-1",
                  "geometry": {"type": "Point", "coordinates": [0, 0]},
                  "properties": {}
                }
              ]
            }
          }
        },
        "layers": [
          {"id":"circle","type":"circle","source":"point","paint":{"circle-radius":["case",["boolean",["feature-state","hover"],false],10,5]}}
        ]
      }
      """;

  @AfterEach
  void restoreProcessState() {
    RenderSessionHandle.resetFrameConstructionFailureForTesting();
    Maplibre.clearLogCallback();
    Maplibre.restoreDefaultAsyncLogSeverities();
  }

  @Test
  void bnd104RejectsNegativeAttachDimensionsBeforeNativeCast() {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        var descriptor =
            new MetalOwnedTextureDescriptor().extent(new RenderTargetExtent(-1, 64, 1.0));
        assertThrows(InvalidArgumentException.class, () -> map.attachMetalOwnedTexture(descriptor));
        if (Maplibre.supportedRenderBackends().contains(RenderBackend.OPENGL)) {
          var openGLDescriptor =
              new OpenGLOwnedTextureDescriptor()
                  .extent(new RenderTargetExtent(-1, 64, 1.0))
                  .context(fakeSupportedOpenGLContext());
          assertThrows(
              InvalidArgumentException.class, () -> map.attachOpenGLOwnedTexture(openGLDescriptor));
        }
      }
    }
  }

  @Test
  void bnd160OpenGLAttachMethodsReportUnsupportedWhenBackendUnavailable() {
    Assumptions.assumeFalse(
        Maplibre.supportedRenderBackends().contains(RenderBackend.OPENGL),
        "OpenGL native build exercises positive attach paths");
    var context = fakeWglContext();
    var extent = new RenderTargetExtent(64, 64, 1.0);
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        assertThrows(
            UnsupportedFeatureException.class,
            () ->
                map.attachOpenGLOwnedTexture(
                    new OpenGLOwnedTextureDescriptor().extent(extent).context(context)));
        assertThrows(
            UnsupportedFeatureException.class,
            () ->
                map.attachOpenGLBorrowedTexture(
                    new OpenGLBorrowedTextureDescriptor()
                        .extent(extent)
                        .context(context)
                        .texture(12)
                        .target(0x0de1)));
        assertThrows(
            UnsupportedFeatureException.class,
            () ->
                map.attachOpenGLSurface(
                    new OpenGLSurfaceDescriptor()
                        .extent(extent)
                        .context(context)
                        .surface(NativePointer.ofAddress(3))));
      }
    }
  }

  @Test
  void bnd160OpenGLAttachMethodsReportNativeValidationErrors() {
    var context = fakeSupportedOpenGLContext();
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        assertThrows(
            InvalidArgumentException.class,
            () ->
                map.attachOpenGLOwnedTexture(
                    new OpenGLOwnedTextureDescriptor()
                        .extent(new RenderTargetExtent(0, 16, 1.0))
                        .context(context)));
        assertThrows(
            InvalidArgumentException.class,
            () ->
                map.attachOpenGLBorrowedTexture(
                    new OpenGLBorrowedTextureDescriptor()
                        .extent(new RenderTargetExtent(32, 16, 1.0))
                        .context(context)
                        .texture(0)
                        .target(0x0de1)));
        assertThrows(
            InvalidArgumentException.class,
            () ->
                map.attachOpenGLSurface(
                    new OpenGLSurfaceDescriptor()
                        .extent(new RenderTargetExtent(32, 16, 1.0))
                        .context(context)
                        .surface(NativePointer.NULL)));
      }
    }
  }

  @Test
  void bnd162AttachAttemptsCrossNativeBoundary() {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        assertThrows(
            MaplibreException.class,
            () -> map.attachMetalOwnedTexture(new MetalOwnedTextureDescriptor()));
        assertThrows(
            MaplibreException.class,
            () -> map.attachMetalBorrowedTexture(new MetalBorrowedTextureDescriptor()));
        assertThrows(
            MaplibreException.class,
            () -> map.attachVulkanOwnedTexture(new VulkanOwnedTextureDescriptor()));
        assertThrows(
            MaplibreException.class,
            () -> map.attachVulkanBorrowedTexture(new VulkanBorrowedTextureDescriptor()));
        assertThrows(
            MaplibreException.class,
            () -> map.attachOpenGLOwnedTexture(new OpenGLOwnedTextureDescriptor()));
        assertThrows(
            MaplibreException.class,
            () -> map.attachOpenGLBorrowedTexture(new OpenGLBorrowedTextureDescriptor()));
        assertThrows(
            MaplibreException.class, () -> map.attachMetalSurface(new MetalSurfaceDescriptor()));
        assertThrows(
            MaplibreException.class, () -> map.attachVulkanSurface(new VulkanSurfaceDescriptor()));
        assertThrows(
            MaplibreException.class, () -> map.attachOpenGLSurface(new OpenGLSurfaceDescriptor()));
      }
    }
  }

  @Test
  void bnd163AttachingSecondRenderSessionReportsInvalidState() throws Exception {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = RenderTargetTestSupport.createSmallMap(runtime)) {
        try (var target = assumeOpenGLOwnedTextureTarget(map)) {
          assertThrows(
              InvalidStateException.class,
              () ->
                  RenderTargetTestSupport.attachOpenGLOwnedTexture(
                      map, new RenderTargetExtent(32, 16, 1.0)));
          assertFalse(target.session().isClosed());
        }
      }
    }
  }

  @Test
  void bnd164ThroughBnd170OpenGLOwnedTextureFrameHandleStaysActiveUntilClosed() throws Exception {
    // BND-165, BND-167, and BND-173 are covered in this owned-texture workflow:
    // resize updates the extent, frame acquisition returns copied metadata, and
    // released frame values reject stale backend-handle access.
    Maplibre.setLogCallback(record -> true);
    Maplibre.setAsyncLogSeverities(Set.of());
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(64, 64));
    try (var target = assumeOpenGLOwnedTextureTarget(map)) {
      var activeSession = target.session();
      map.setStyleJson(STYLE_JSON);
      waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE);
      activeSession.renderUpdate();
      OpenGLOwnedTextureFrame frame;
      try (var frameHandle = activeSession.acquireOpenGLOwnedTextureFrame()) {
        frame = frameHandle.frame();
        assertEquals(32, frame.width());
        assertEquals(0x0de1, frame.target());
        assertEquals(0x8058, frame.internalFormat());
        assertEquals(0x1908, frame.format());
        assertEquals(0x1401, frame.type());
        assertTrue(frame.texture() != 0);
        assertFalse(frameHandle.isClosed());
        assertThrows(InvalidStateException.class, activeSession::acquireOpenGLOwnedTextureFrame);
        assertThrows(InvalidStateException.class, activeSession::textureImageInfo);
        try (var buffer = NativeBuffer.allocate(4)) {
          assertThrows(
              InvalidStateException.class, () -> activeSession.readPremultipliedRgba8(buffer));
        }
        assertThrows(InvalidStateException.class, () -> activeSession.resize(32, 16, 1.0));
        assertThrows(InvalidStateException.class, activeSession::renderUpdate);
        assertThrows(InvalidStateException.class, activeSession::detach);
      }
      assertThrows(IllegalStateException.class, frame::texture);
      var info = activeSession.textureImageInfo();
      assertEquals(32, info.width());
      assertEquals(16, info.height());
      assertTrue(info.byteLength() > 4);
      try (var buffer = NativeBuffer.allocate(info.byteLength() - 1)) {
        assertThrows(
            InvalidArgumentException.class, () -> activeSession.readPremultipliedRgba8(buffer));
        assertEquals(info.byteLength() - 1, buffer.byteLength());
      }
      try (var buffer = NativeBuffer.allocate(info.byteLength())) {
        assertEquals(info, activeSession.readPremultipliedRgba8(buffer));
        assertTrue(hasNonZeroByte(buffer.toByteArray()));
      }
      activeSession.resize(48, 24, 1.0);
      map.setStyleJson(STYLE_JSON);
      waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE);
      activeSession.renderUpdate();
      try (var frameHandle = activeSession.acquireOpenGLOwnedTextureFrame()) {
        var resizedFrame = frameHandle.frame();
        assertEquals(48, resizedFrame.width());
        assertEquals(24, resizedFrame.height());
      }
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void bnd169OpenGLOwnedTextureFrameCloseFailureLeavesHandleRetryable() throws Exception {
    Maplibre.setLogCallback(record -> true);
    Maplibre.setAsyncLogSeverities(Set.of());
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(64, 64));
    try (var target = assumeOpenGLOwnedTextureTarget(map)) {
      var activeSession = target.session();
      map.setStyleJson(STYLE_JSON);
      waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE);
      activeSession.renderUpdate();
      var frameHandle = activeSession.acquireOpenGLOwnedTextureFrame();
      var frame = frameHandle.frame();
      try {
        assertWrongThread(runOnOtherThread(frameHandle::close));
        assertFalse(frameHandle.isClosed());
        assertTrue(frame.texture() != 0);
        assertThrows(InvalidStateException.class, activeSession::renderUpdate);
      } finally {
        frameHandle.close();
      }
      assertTrue(frameHandle.isClosed());
      assertThrows(IllegalStateException.class, frame::texture);
      activeSession.renderUpdate();
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void bnd172FrameConstructionFailureReleasesNativeFrame() throws Exception {
    Maplibre.setLogCallback(record -> true);
    Maplibre.setAsyncLogSeverities(Set.of());
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(64, 64));
    try (var target = assumeOpenGLOwnedTextureTarget(map)) {
      var activeSession = target.session();
      map.setStyleJson(STYLE_JSON);
      waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE);
      activeSession.renderUpdate();
      var failure = new IllegalStateException("injected frame construction failure");
      RenderSessionHandle.failNextFrameConstructionForTesting(failure);
      assertSame(
          failure,
          assertThrows(IllegalStateException.class, activeSession::acquireOpenGLOwnedTextureFrame));
      try (var frameHandle = activeSession.acquireOpenGLOwnedTextureFrame()) {
        assertTrue(frameHandle.frame().texture() != 0);
      }
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void bnd066AndBnd105FeatureStateSetGetAndRemoveCopiesSnapshots() throws Exception {
    Maplibre.setLogCallback(record -> true);
    Maplibre.setAsyncLogSeverities(Set.of());
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(64, 64));
    try (var target = assumeOpenGLOwnedTextureTarget(map)) {
      var activeSession = target.session();
      var selector = new FeatureStateSelector("point").featureId("feature-1");
      var state =
          JsonValue.object(
              List.of(
                  new JsonValue.Member("hover", JsonValue.of(true)),
                  new JsonValue.Member("radius", JsonValue.unsigned(20))));

      assertThrows(
          InvalidStateException.class, () -> activeSession.setFeatureState(selector, state));
      assertThrows(
          IllegalStateException.class, () -> new FeatureStateSelector("point").stateKey("hover"));

      map.setStyleJson(FEATURE_STATE_STYLE_JSON);
      waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE);
      activeSession.renderUpdate();
      assertThrows(
          InvalidArgumentException.class,
          () -> activeSession.setFeatureState(selector, JsonValue.array(List.of())));

      activeSession.setFeatureState(selector, state);
      QueryStructs.resetJsonSnapshotDestroyCountForTesting();
      var snapshotFailure = new IllegalStateException("injected JSON snapshot copy failure");
      QueryStructs.failNextJsonSnapshotCopyForTesting(snapshotFailure);
      assertSame(
          snapshotFailure,
          assertThrows(IllegalStateException.class, () -> activeSession.getFeatureState(selector)));
      assertEquals(1, QueryStructs.jsonSnapshotDestroyCountForTesting());

      var copied =
          assertInstanceOf(JsonValue.ObjectValue.class, activeSession.getFeatureState(selector));
      assertEquals(JsonValue.of(true), member(copied, "hover"));
      assertEquals(JsonValue.unsigned(20), member(copied, "radius"));

      activeSession.removeFeatureState(
          new FeatureStateSelector("point").featureId("feature-1").stateKey("hover"));
      renderIfAvailable(runtime, map, activeSession);
      var afterRemove =
          assertInstanceOf(JsonValue.ObjectValue.class, activeSession.getFeatureState(selector));
      assertEquals(JsonValue.unsigned(20), member(afterRemove, "radius"));
      assertFalse(hasMember(afterRemove, "hover"));
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void bnd066AndBnd106RenderedAndSourceQueriesCopyResults() throws Exception {
    Maplibre.setLogCallback(record -> true);
    Maplibre.setAsyncLogSeverities(Set.of());
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(64, 64));
    try (var target = assumeOpenGLOwnedTextureTarget(map, new RenderTargetExtent(64, 64, 1.0))) {
      var activeSession = target.session();
      assertThrows(
          InvalidStateException.class,
          () ->
              activeSession.queryRenderedFeatures(
                  RenderedQueryGeometry.point(new ScreenPoint(32, 32))));

      loadQueryStyleAndRender(runtime, map, activeSession);
      activeSession.setFeatureState(
          new FeatureStateSelector("point").featureId("feature-1"),
          JsonValue.object(List.of(new JsonValue.Member("selected", JsonValue.of(true)))));
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
          waitForRenderedFeature(
              runtime,
              map,
              activeSession,
              geometry,
              new RenderedFeatureQueryOptions().layerIds(List.of("point-circle")).filter(filter),
              "rendered point feature");
      assertEquals("point", rendered.sourceId().orElseThrow());
      assertTrue(rendered.sourceLayerId().isEmpty());
      assertEquals(FeatureIdentifier.of("feature-1"), rendered.feature().identifier());
      var renderedPoint = assertInstanceOf(Geometry.Point.class, rendered.feature().geometry());
      assertEquals(37.7749, renderedPoint.coordinate().latitude(), QUERY_COORDINATE_TOLERANCE);
      assertEquals(-122.4194, renderedPoint.coordinate().longitude(), QUERY_COORDINATE_TOLERANCE);
      assertEquals(JsonValue.of("capital"), member(rendered.feature(), "kind"));
      assertEquals(
          JsonValue.of(true),
          member(
              assertInstanceOf(JsonValue.ObjectValue.class, rendered.state().orElseThrow()),
              "selected"));
      QueryStructs.resetFeatureQueryResultDestroyCountForTesting();
      var resultFailure = new IllegalStateException("injected query result copy failure");
      QueryStructs.failNextFeatureQueryResultCopyForTesting(resultFailure);
      assertSame(
          resultFailure,
          assertThrows(
              IllegalStateException.class,
              () ->
                  activeSession.queryRenderedFeatures(
                      geometry,
                      new RenderedFeatureQueryOptions().layerIds(List.of("point-circle")))));
      assertEquals(1, QueryStructs.featureQueryResultDestroyCountForTesting());

      var source =
          waitForSourceFeature(
              runtime,
              map,
              activeSession,
              "point",
              new SourceFeatureQueryOptions().filter(filter),
              "source point feature");
      assertEquals("point", source.sourceId().orElseThrow());
      assertTrue(source.sourceLayerId().isEmpty());
      assertEquals(FeatureIdentifier.of("feature-1"), source.feature().identifier());
      var sourcePoint = assertInstanceOf(Geometry.Point.class, source.feature().geometry());
      assertEquals(37.7749, sourcePoint.coordinate().latitude(), QUERY_COORDINATE_TOLERANCE);
      assertEquals(-122.4194, sourcePoint.coordinate().longitude(), QUERY_COORDINATE_TOLERANCE);
      assertEquals(JsonValue.of("capital"), member(source.feature(), "kind"));
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void bnd066FeatureExtensionQueriesCopyValueAndFeatureCollectionResults() throws Exception {
    Maplibre.setLogCallback(record -> true);
    Maplibre.setAsyncLogSeverities(Set.of());
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(64, 64));
    try (var target = assumeOpenGLOwnedTextureTarget(map, new RenderTargetExtent(64, 64, 1.0))) {
      var activeSession = target.session();
      loadClusterStyleAndRender(runtime, map, activeSession);
      var queryPoint = map.pixelForLatLng(new LatLng(0.0, 0.0));
      var geometry =
          RenderedQueryGeometry.box(
              new ScreenBox(
                  new ScreenPoint(queryPoint.x() - 30.0, queryPoint.y() - 30.0),
                  new ScreenPoint(queryPoint.x() + 30.0, queryPoint.y() + 30.0)));
      var cluster =
          waitForRenderedFeature(
              runtime,
              map,
              activeSession,
              geometry,
              new RenderedFeatureQueryOptions().layerIds(List.of("cluster-circle")),
              "rendered cluster");

      var children =
          assertInstanceOf(
              FeatureExtensionResult.FeatureCollection.class,
              activeSession.queryFeatureExtension(
                  "cluster-source", cluster.feature(), "supercluster", "children"));
      assertTrue(children.features().size() > 0);

      var expansionZoom =
          assertInstanceOf(
              FeatureExtensionResult.Value.class,
              activeSession.queryFeatureExtension(
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
              activeSession.queryFeatureExtension(
                  "cluster-source", cluster.feature(), "supercluster", "leaves", leavesArguments));
      assertEquals(1, leaves.features().size());
      QueryStructs.resetFeatureExtensionResultDestroyCountForTesting();
      var resultFailure = new IllegalStateException("injected feature extension copy failure");
      QueryStructs.failNextFeatureExtensionResultCopyForTesting(resultFailure);
      assertSame(
          resultFailure,
          assertThrows(
              IllegalStateException.class,
              () ->
                  activeSession.queryFeatureExtension(
                      "cluster-source",
                      cluster.feature(),
                      "supercluster",
                      "leaves",
                      leavesArguments)));
      assertEquals(1, QueryStructs.featureExtensionResultDestroyCountForTesting());
      assertThrows(
          InvalidArgumentException.class,
          () ->
              activeSession.queryFeatureExtension(
                  "cluster-source",
                  cluster.feature(),
                  "supercluster",
                  "leaves",
                  JsonValue.array(List.of())));
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void bnd162AndBnd171OpenGLBorrowedTextureSessionRendersThroughPublicBinding() throws Exception {
    Maplibre.setLogCallback(record -> true);
    Maplibre.setAsyncLogSeverities(Set.of());
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(128, 128));
    try (var target = assumeOpenGLBorrowedTextureTarget(map)) {
      var activeSession = target.session();
      map.setStyleJson(STYLE_JSON);
      waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE);
      activeSession.renderUpdate();
      assertThrows(
          UnsupportedFeatureException.class, activeSession::acquireOpenGLOwnedTextureFrame);
      assertThrows(UnsupportedFeatureException.class, activeSession::textureImageInfo);
      try (var buffer = NativeBuffer.allocate(4)) {
        assertThrows(
            UnsupportedFeatureException.class, () -> activeSession.readPremultipliedRgba8(buffer));
      }
      assertThrows(UnsupportedFeatureException.class, () -> activeSession.resize(128, 128, 1.0));
      var renderedBytes = target.readOpenGLBorrowedTextureRgba();
      assertTrue(hasNonZeroByte(renderedBytes));
      activeSession.close();
      assertArrayEquals(renderedBytes, target.readOpenGLBorrowedTextureRgba());
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void bnd162OpenGLSurfaceSessionRendersThroughPublicBinding() throws Exception {
    Maplibre.setLogCallback(record -> true);
    Maplibre.setAsyncLogSeverities(Set.of());
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(128, 128));
    try (var target = assumeOpenGLSurfaceTarget(map)) {
      var activeSession = target.session();
      map.setStyleJson(STYLE_JSON);
      waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE);
      activeSession.renderUpdate();
      assertThrows(
          UnsupportedFeatureException.class, activeSession::acquireOpenGLOwnedTextureFrame);
      assertThrows(UnsupportedFeatureException.class, activeSession::textureImageInfo);
      try (var buffer = NativeBuffer.allocate(4)) {
        assertThrows(
            UnsupportedFeatureException.class, () -> activeSession.readPremultipliedRgba8(buffer));
      }
      assertTrue(hasNonZeroByte(target.readOpenGLSurfaceRgba(128, 128)));
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void bnd161OpenGLDescriptorAccessorsRoundTrip() {
    var extent = new RenderTargetExtent(32, 16, 2.0);
    var eglContext =
        new EglContextDescriptor(
                NativePointer.ofAddress(0x10),
                NativePointer.ofAddress(0x20),
                NativePointer.ofAddress(0x30))
            .getProcAddress(NativePointer.ofAddress(0x40));
    var wglContext =
        new WglContextDescriptor(NativePointer.ofAddress(0x50), NativePointer.ofAddress(0x60))
            .getProcAddress(NativePointer.ofAddress(0x70));
    var owned = new OpenGLOwnedTextureDescriptor().extent(extent).context(eglContext);
    assertEquals(extent, owned.extent());
    assertEquals(eglContext, owned.context());
    var borrowed =
        new OpenGLBorrowedTextureDescriptor()
            .extent(extent)
            .context(wglContext)
            .texture(12)
            .target(0x0de1);
    assertEquals(extent, borrowed.extent());
    assertEquals(wglContext, borrowed.context());
    assertEquals(12, borrowed.texture());
    assertEquals(0x0de1, borrowed.target());
    var surface =
        new OpenGLSurfaceDescriptor()
            .extent(extent)
            .context(wglContext)
            .surface(NativePointer.ofAddress(0x80));
    assertEquals(extent, surface.extent());
    assertEquals(wglContext, surface.context());
    assertEquals(NativePointer.ofAddress(0x80), surface.surface());
  }

  @Test
  void bnd168OpenGLOwnedTextureFrameValuesExpireWithScope() {
    var scope = new FrameScope();
    var frame =
        new OpenGLOwnedTextureFrame(scope, 1, 2, 3, 1.0, 4, 5, 0x0de1, 0x8058, 0x1908, 0x1401);
    assertEquals(5, frame.texture());
    scope.close();
    assertThrows(IllegalStateException.class, frame::texture);
  }

  @Test
  void bnd160SupportedOpenGLContextProviderMaskMapsFlags() {
    assertTrue(OpenGLContextProvider.fromMask(0).isEmpty());
    assertEquals(
        java.util.EnumSet.of(OpenGLContextProvider.WGL), OpenGLContextProvider.fromMask(1));
    assertEquals(
        java.util.EnumSet.of(OpenGLContextProvider.WGL, OpenGLContextProvider.EGL),
        OpenGLContextProvider.fromMask(3));
  }

  private static RenderTargetTestSupport assumeOpenGLOwnedTextureTarget(MapHandle map) {
    return assumeOpenGLOwnedTextureTarget(map, new RenderTargetExtent(32, 16, 1.0));
  }

  private static RenderTargetTestSupport assumeOpenGLOwnedTextureTarget(
      MapHandle map, RenderTargetExtent extent) {
    Assumptions.assumeTrue(
        Maplibre.supportedRenderBackends().contains(RenderBackend.OPENGL),
        "OpenGL owned texture unavailable in this native build");
    try {
      return RenderTargetTestSupport.attachOpenGLOwnedTexture(map, extent);
    } catch (MaplibreException | IllegalStateException error) {
      return fail("OpenGL owned texture unavailable: " + error.getMessage(), error);
    }
  }

  private static RenderTargetTestSupport assumeOpenGLBorrowedTextureTarget(MapHandle map) {
    Assumptions.assumeTrue(
        Maplibre.supportedRenderBackends().contains(RenderBackend.OPENGL),
        "OpenGL borrowed texture unavailable in this native build");
    try {
      return RenderTargetTestSupport.attachOpenGLBorrowedTexture(
          map, new RenderTargetExtent(128, 128, 1.0));
    } catch (MaplibreException | IllegalStateException error) {
      return fail("OpenGL borrowed texture unavailable: " + error.getMessage(), error);
    }
  }

  private static RenderTargetTestSupport assumeOpenGLSurfaceTarget(MapHandle map) {
    Assumptions.assumeTrue(
        Maplibre.supportedRenderBackends().contains(RenderBackend.OPENGL),
        "OpenGL surface unavailable in this native build");
    try {
      return RenderTargetTestSupport.attachOpenGLSurface(
          map, new RenderTargetExtent(128, 128, 1.0));
    } catch (MaplibreException | IllegalStateException error) {
      return fail("OpenGL surface unavailable: " + error.getMessage(), error);
    }
  }

  private static WglContextDescriptor fakeWglContext() {
    var fake = NativePointer.ofAddress(1);
    return new WglContextDescriptor(fake, fake);
  }

  private static OpenGLContextDescriptor fakeSupportedOpenGLContext() {
    var fake = NativePointer.ofAddress(1);
    var providers = Maplibre.supportedOpenGLContextProviders();
    if (providers.contains(OpenGLContextProvider.EGL)) {
      return new EglContextDescriptor(fake, fake, fake);
    }
    if (providers.contains(OpenGLContextProvider.WGL)) {
      return new WglContextDescriptor(fake, fake);
    }
    return fakeWglContext();
  }

  private static boolean hasNonZeroByte(byte[] bytes) {
    for (var value : bytes) {
      if (value != 0) {
        return true;
      }
    }
    return false;
  }

  private static void loadQueryStyleAndRender(
      RuntimeHandle runtime, MapHandle map, RenderSessionHandle session) {
    map.jumpTo(new CameraOptions().center(37.7749, -122.4194).zoom(10.0));
    map.setStyleJson(QUERY_STYLE_JSON);
    renderAvailableUpdates(runtime, map, session, 5);
  }

  private static void loadClusterStyleAndRender(
      RuntimeHandle runtime, MapHandle map, RenderSessionHandle session) {
    map.jumpTo(new CameraOptions().center(0.0, 0.0).zoom(0.0));
    map.setStyleJson(CLUSTER_STYLE_JSON);
    renderAvailableUpdates(runtime, map, session, 5);
  }

  private static void renderAvailableUpdates(
      RuntimeHandle runtime, MapHandle map, RenderSessionHandle session, int count) {
    for (var i = 0; i < count; i++) {
      if (waitForMapEventIfAvailable(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE)) {
        try {
          session.renderUpdate();
        } catch (InvalidStateException ignored) {
          // A pending event may be stale after another update.
        }
      }
    }
  }

  private static void renderIfAvailable(
      RuntimeHandle runtime, MapHandle map, RenderSessionHandle session)
      throws InterruptedException {
    waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE);
    try {
      session.renderUpdate();
    } catch (InvalidStateException ignored) {
      // A render-update event can be stale after another update has already rendered.
    }
  }

  private static void renderPendingUpdates(
      RuntimeHandle runtime, MapHandle map, RenderSessionHandle session) {
    runtime.runOnce();
    for (var eventCount = 0; eventCount < 100; eventCount++) {
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

  private static QueriedFeature waitForRenderedFeature(
      RuntimeHandle runtime,
      MapHandle map,
      RenderSessionHandle session,
      RenderedQueryGeometry geometry,
      RenderedFeatureQueryOptions options,
      String description)
      throws InterruptedException {
    for (var attempt = 0; attempt < 1000; attempt++) {
      var features = session.queryRenderedFeatures(geometry, options);
      if (features.size() == 1) {
        return features.get(0);
      }
      renderPendingUpdates(runtime, map, session);
      Thread.sleep(1);
    }
    fail("Timed out waiting for " + description);
    return null;
  }

  private static QueriedFeature waitForSourceFeature(
      RuntimeHandle runtime,
      MapHandle map,
      RenderSessionHandle session,
      String sourceId,
      SourceFeatureQueryOptions options,
      String description)
      throws InterruptedException {
    for (var attempt = 0; attempt < 1000; attempt++) {
      var features = session.querySourceFeatures(sourceId, options);
      if (features.size() == 1) {
        return features.get(0);
      }
      renderPendingUpdates(runtime, map, session);
      Thread.sleep(1);
    }
    fail("Timed out waiting for " + description);
    return null;
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

  private static boolean waitForMapEventIfAvailable(
      RuntimeHandle runtime, MapHandle map, RuntimeEventType type) {
    for (var attempt = 0; attempt < 1000; attempt++) {
      runtime.runOnce();
      while (true) {
        var event = runtime.pollEvent();
        if (event.isEmpty()) {
          break;
        }
        var value = event.get();
        if (value.type() == type && value.mapSource().filter(source -> source == map).isPresent()) {
          return true;
        }
      }
      try {
        Thread.sleep(1);
      } catch (InterruptedException exception) {
        Thread.currentThread().interrupt();
        return false;
      }
    }
    return false;
  }

  @FunctionalInterface
  private interface ThrowingRunnable {
    void run() throws Exception;
  }
}
