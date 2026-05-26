package org.maplibre.nativejni;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.EnumSet;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.error.InvalidArgumentException;
import org.maplibre.nativejni.error.MaplibreStatus;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.internal.status.Status;
import org.maplibre.nativejni.log.LogSeverity;
import org.maplibre.nativejni.runtime.NetworkStatus;
import org.maplibre.nativejni.test.NativeTestSupport;

class MaplibreTest {
  private static NetworkStatus originalNetworkStatus;

  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibraryOrSkip();
    originalNetworkStatus = Maplibre.networkStatus();
  }

  @AfterEach
  void restoreProcessState() {
    if (originalNetworkStatus != null) {
      Maplibre.setNetworkStatus(originalNetworkStatus);
    }
    Maplibre.clearLogCallback();
    Maplibre.restoreDefaultAsyncLogSeverities();
  }

  @Test
  void readsCAbiVersion() {
    assertTrue(Maplibre.cVersion() >= 0);
  }

  @Test
  void readsSupportedRenderBackends() {
    assertEquals(
        Maplibre.supportedRenderBackends(),
        org.maplibre.nativejni.render.RenderBackend.fromMask(
            MaplibreNativeC.mln_supported_render_backend_mask()));
  }

  @Test
  void roundTripsNetworkStatus() {
    Maplibre.setNetworkStatus(NetworkStatus.OFFLINE);
    assertEquals(NetworkStatus.OFFLINE, Maplibre.networkStatus());

    Maplibre.setNetworkStatus(NetworkStatus.ONLINE);
    assertEquals(NetworkStatus.ONLINE, Maplibre.networkStatus());
  }

  @Test
  void roundTripsProjectedMeters() {
    var coordinate = new LatLng(37.7749, -122.4194);

    var meters = Maplibre.projectedMetersForLatLng(coordinate);
    var roundTrip = Maplibre.latLngForProjectedMeters(meters);

    assertEquals(coordinate.latitude(), roundTrip.latitude(), 1.0e-9);
    assertEquals(coordinate.longitude(), roundTrip.longitude(), 1.0e-9);
  }

  @Test
  void installsAndClearsLogCallback() {
    Maplibre.setLogCallback(record -> true);
    Maplibre.clearLogCallback();
  }

  @Test
  void configuresAsyncLogSeverities() {
    Maplibre.setAsyncLogSeverities(EnumSet.of(LogSeverity.ERROR));
    Maplibre.restoreDefaultAsyncLogSeverities();
  }

  @Test
  void convertsNativeStatusAndCapturesDiagnostic() {
    var exception =
        assertThrows(
            InvalidArgumentException.class,
            () -> Status.check(MaplibreNativeC.mln_network_status_set(999_999)));

    assertEquals(MaplibreStatus.INVALID_ARGUMENT, exception.status());
    assertEquals(MaplibreStatus.INVALID_ARGUMENT.nativeCode(), exception.nativeStatusCode());
    assertFalse(exception.diagnostic().isBlank());
    assertTrue(exception.diagnostic().contains("network status"));
  }
}
