package org.maplibre.nativejni.map;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.List;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.camera.CameraOptions;
import org.maplibre.nativejni.camera.EdgeInsets;
import org.maplibre.nativejni.geo.Geometry;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.runtime.RuntimeHandle;

class MapProjectionHandleTest {
  @Test
  void projectionConvertsPixelsAndCoordinates() {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        try (var projection = map.createProjection()) {
          var coordinate = new LatLng(0, 0);
          var point = projection.pixelForLatLng(coordinate);
          var roundTrip = projection.latLngForPixel(point);
          assertEquals(coordinate.latitude(), roundTrip.latitude(), 1.0e-9);
          assertEquals(coordinate.longitude(), roundTrip.longitude(), 1.0e-9);
        }
      }
    }
  }

  @Test
  void projectionCameraCanBeReadSetAndFitToCoordinates() {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(256, 256))) {
        try (var projection = map.createProjection()) {
          projection.setCamera(new CameraOptions().center(10, 20).zoom(3).bearing(15).pitch(20));
          var camera = projection.camera();
          assertTrue(camera.hasCenter());
          assertEquals(10, camera.center().latitude(), 1.0e-9);
          assertEquals(20, camera.center().longitude(), 1.0e-9);
          assertEquals(3, camera.zoom(), 1.0e-9);
          assertEquals(15, camera.bearing(), 1.0e-9);
          assertEquals(20, camera.pitch(), 1.0e-9);
          projection.setVisibleCoordinates(
              List.of(new LatLng(-1, -1), new LatLng(1, 1)), new EdgeInsets(0, 0, 0, 0));
          assertTrue(projection.camera().hasCenter());
          projection.setVisibleGeometry(
              Geometry.lineString(List.of(new LatLng(-2, -2), new LatLng(2, 2))),
              new EdgeInsets(0, 0, 0, 0));
          assertTrue(projection.camera().hasCenter());
        }
      }
    }
  }

  @Test
  void projectionOwnsStandaloneNativeSnapshot() {
    try (var runtime = RuntimeHandle.create()) {
      var map = MapHandle.create(runtime, new MapOptions().size(64, 64));
      var projection = map.createProjection();
      assertFalse(projection.isClosed());
      map.close();
      assertTrue(map.isClosed());
      projection.close();
      assertTrue(projection.isClosed());
      projection.close();
    }
  }
}
