package org.maplibre.nativejni.internal.struct;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.camera.AnimationOptions;
import org.maplibre.nativejni.camera.BoundOptions;
import org.maplibre.nativejni.camera.CameraFitOptions;
import org.maplibre.nativejni.camera.CameraOptions;
import org.maplibre.nativejni.camera.EdgeInsets;
import org.maplibre.nativejni.camera.FreeCameraOptions;
import org.maplibre.nativejni.camera.UnitBezier;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.geo.LatLngBounds;
import org.maplibre.nativejni.geo.Quaternion;
import org.maplibre.nativejni.geo.ScreenBox;
import org.maplibre.nativejni.geo.ScreenPoint;
import org.maplibre.nativejni.geo.Vec3;
import org.maplibre.nativejni.map.MapMode;
import org.maplibre.nativejni.map.MapOptions;
import org.maplibre.nativejni.resource.ResourceErrorReason;
import org.maplibre.nativejni.resource.ResourceResponse;
import org.maplibre.nativejni.runtime.RuntimeOptions;

final class DescriptorStructsTest {
  @Test
  void coreCopiedValuesRoundTrip() {
    assertEquals(new LatLng(1, 2), CoreStructs.latLng(CoreStructs.latLng(new LatLng(1, 2))));
    assertEquals(
        new LatLngBounds(new LatLng(-1, -2), new LatLng(3, 4)),
        CoreStructs.latLngBounds(
            CoreStructs.latLngBounds(new LatLngBounds(new LatLng(-1, -2), new LatLng(3, 4)))));
    assertEquals(
        new ScreenPoint(5, 6),
        CoreStructs.screenPoint(CoreStructs.screenPoint(new ScreenPoint(5, 6))));
    assertEquals(
        new ScreenBox(new ScreenPoint(1, 2), new ScreenPoint(3, 4)),
        CoreStructs.screenBox(
            CoreStructs.screenBox(new ScreenBox(new ScreenPoint(1, 2), new ScreenPoint(3, 4)))));
    assertEquals(new Vec3(1, 2, 3), CoreStructs.vec3(CoreStructs.vec3(new Vec3(1, 2, 3))));
    assertEquals(
        new Quaternion(1, 2, 3, 4),
        CoreStructs.quaternion(CoreStructs.quaternion(new Quaternion(1, 2, 3, 4))));
  }

  @Test
  void mapAndCameraDescriptorsMaterializeFields() {
    var map =
        MapStructs.mapOptions(
            new MapOptions().size(320, 240).scaleFactor(2).mapMode(MapMode.STATIC));
    assertEquals(320, map.width());
    assertEquals(240, map.height());
    assertEquals(2.0, map.scaleFactor(), 0.0);
    assertEquals(MapMode.STATIC.nativeValue(), map.mapMode());

    var camera =
        MapStructs.cameraOptions(
            new CameraOptions()
                .center(1, 2)
                .centerAltitude(3)
                .padding(EdgeInsets.ZERO)
                .anchor(new ScreenPoint(4, 5))
                .zoom(6)
                .bearing(7)
                .pitch(8)
                .roll(9)
                .fieldOfView(10));
    assertTrue(camera.hasCenter());
    assertEquals(1, camera.center().latitude(), 0.0);
    assertTrue(camera.hasFieldOfView());
    assertEquals(10, camera.fieldOfView(), 0.0);

    var animation =
        MapStructs.animationOptions(
            new AnimationOptions()
                .durationMs(100)
                .velocity(2)
                .minZoom(3)
                .easing(new UnitBezier(0, 0, 1, 1)));
    assertTrue(animation.hasEasing());
    assertEquals(1, animation.easing().x2(), 0.0);

    var fit =
        MapStructs.cameraFitOptions(
            new CameraFitOptions().padding(EdgeInsets.ZERO).bearing(1).pitch(2));
    assertTrue(fit.hasPadding());
    assertEquals(2, fit.pitch(), 0.0);

    var bounds =
        MapStructs.boundOptions(
            new BoundOptions()
                .bounds(new LatLngBounds(new LatLng(-1, -2), new LatLng(3, 4)))
                .minZoom(0)
                .maxZoom(22));
    assertTrue(bounds.hasBounds());
    assertEquals(22, bounds.maxZoom(), 0.0);

    var free =
        MapStructs.freeCameraOptions(
            new FreeCameraOptions()
                .position(new Vec3(1, 2, 3))
                .orientation(new Quaternion(0, 0, 0, 1)));
    assertTrue(free.hasPosition());
    assertEquals(1, free.orientation().w(), 0.0);
  }

  @Test
  void queryRuntimeResourceAndStyleDescriptorsMaterializeFields() {
    var runtime =
        RuntimeStructs.runtimeOptions(
            new RuntimeOptions().assetPath("assets").cachePath("cache").maximumCacheSize(42));
    assertEquals("assets", runtime.assetPath());
    assertEquals("cache", runtime.cachePath());
    assertTrue(runtime.hasMaximumCacheSize());
    assertEquals(42, runtime.maximumCacheSize());

    var response =
        ResourceStructs.resourceResponse(
            ResourceResponse.error(ResourceErrorReason.NOT_FOUND, "missing")
                .mustRevalidate(true)
                .etag("abc"));
    assertEquals(ResourceErrorReason.NOT_FOUND.nativeValue(), response.errorReason());
    assertEquals("missing", response.errorMessage());
    assertEquals("abc", response.etag());
  }

  @Test
  void enumConversionsPreserveKnownAndUnknownValues() {
    assertEquals(MapMode.TILE.nativeValue(), 2);
    assertEquals(ResourceErrorReason.NOT_FOUND, ResourceErrorReason.fromNative(1));
    assertEquals(ResourceErrorReason.UNKNOWN, ResourceErrorReason.fromNative(999));
    assertFalse(
        org.maplibre.nativejni.render.RenderBackend.fromMask(0)
            .contains(org.maplibre.nativejni.render.RenderBackend.METAL));
  }
}
