package org.maplibre.nativeffi;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertThrows;

import java.util.EnumSet;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.geo.LatLng;
import org.maplibre.nativeffi.log.LogSeverity;
import org.maplibre.nativeffi.runtime.NetworkStatus;
import org.maplibre.nativeffi.test.NativeTestSupport;

final class MaplibreTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibrary();
  }

  @AfterEach
  void restoreProcessState() {
    Maplibre.clearLogCallback();
    Maplibre.restoreDefaultAsyncLogSeverities();
  }

  @Test
  void exposesCVersionAndSupportedBackends() {
    assertEquals(0, Maplibre.cVersion());
    assertNotNull(Maplibre.supportedRenderBackends());
  }

  @Test
  void getsAndSetsNetworkStatus() {
    var original = Maplibre.networkStatus();
    try {
      Maplibre.setNetworkStatus(NetworkStatus.OFFLINE);
      assertEquals(NetworkStatus.OFFLINE, Maplibre.networkStatus());
      Maplibre.setNetworkStatus(NetworkStatus.ONLINE);
      assertEquals(NetworkStatus.ONLINE, Maplibre.networkStatus());
    } finally {
      Maplibre.setNetworkStatus(original);
    }
  }

  @Test
  void configuresAsyncLogSeverities() {
    Maplibre.setAsyncLogSeverities(EnumSet.noneOf(LogSeverity.class));
    Maplibre.setAsyncLogSeverities(EnumSet.of(LogSeverity.INFO, LogSeverity.WARNING));
    assertThrows(
        IllegalArgumentException.class,
        () -> Maplibre.setAsyncLogSeverities(EnumSet.of(LogSeverity.UNKNOWN)));
    Maplibre.restoreDefaultAsyncLogSeverities();
  }

  @Test
  void convertsProjectedMeters() {
    var meters = Maplibre.projectedMetersForLatLng(new LatLng(0, 0));
    assertEquals(0.0, meters.northing(), 1e-9);
    assertEquals(0.0, meters.easting(), 1e-9);
    var coordinate = Maplibre.latLngForProjectedMeters(meters);
    assertEquals(0.0, coordinate.latitude(), 1e-9);
    assertEquals(0.0, coordinate.longitude(), 1e-9);
  }

  @Test
  void loggingCallbackCanBeInstalledAndCleared() {
    Maplibre.setLogCallback(record -> true);
    Maplibre.clearLogCallback();
  }
}
