package org.maplibre.nativejni.map;

import java.util.List;
import org.maplibre.nativejni.camera.CameraOptions;
import org.maplibre.nativejni.camera.EdgeInsets;
import org.maplibre.nativejni.geo.Geometry;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.geo.ScreenPoint;

/** API-parity scaffold for the Java JNI binding. */
public final class MapProjectionHandle implements AutoCloseable {
  private static UnsupportedOperationException unsupported() {
    return new UnsupportedOperationException(
        "MapProjectionHandle is not implemented by the JNI bridge yet");
  }

  public static MapProjectionHandle create(MapHandle map) {
    throw unsupported();
  }

  public CameraOptions camera() {
    throw unsupported();
  }

  public void setCamera(CameraOptions camera) {
    throw unsupported();
  }

  public void setVisibleCoordinates(List<LatLng> coordinates, EdgeInsets padding) {
    throw unsupported();
  }

  public void setVisibleGeometry(Geometry geometry, EdgeInsets padding) {
    throw unsupported();
  }

  public ScreenPoint pixelForLatLng(LatLng coordinate) {
    throw unsupported();
  }

  public LatLng latLngForPixel(ScreenPoint point) {
    throw unsupported();
  }

  public void close() {
    throw unsupported();
  }

  public boolean isClosed() {
    throw unsupported();
  }
}
