package org.maplibre.nativeffi;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertThrows;

import java.util.EnumSet;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.internal.NativeTestSupport;

final class MapLibreTest {
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
  void exposesCVersionAndSupportedBackends() {
    assertEquals(0, MapLibre.cVersion());
    assertNotNull(MapLibre.supportedRenderBackends());
  }

  @Test
  void getsAndSetsNetworkStatus() {
    var original = MapLibre.networkStatus();
    try {
      MapLibre.setNetworkStatus(NetworkStatus.OFFLINE);
      assertEquals(NetworkStatus.OFFLINE, MapLibre.networkStatus());
      MapLibre.setNetworkStatus(NetworkStatus.ONLINE);
      assertEquals(NetworkStatus.ONLINE, MapLibre.networkStatus());
    } finally {
      MapLibre.setNetworkStatus(original);
    }
  }

  @Test
  void configuresAsyncLogSeverities() {
    MapLibre.setAsyncLogSeverities(EnumSet.noneOf(LogSeverity.class));
    MapLibre.setAsyncLogSeverities(EnumSet.of(LogSeverity.INFO, LogSeverity.WARNING));
    assertThrows(
        IllegalArgumentException.class,
        () -> MapLibre.setAsyncLogSeverities(EnumSet.of(LogSeverity.UNKNOWN)));
    MapLibre.restoreDefaultAsyncLogSeverities();
  }

  @Test
  void convertsProjectedMeters() {
    var meters = MapLibre.projectedMetersForLatLng(new LatLng(0, 0));
    assertEquals(0.0, meters.northing(), 1e-9);
    assertEquals(0.0, meters.easting(), 1e-9);
    var coordinate = MapLibre.latLngForProjectedMeters(meters);
    assertEquals(0.0, coordinate.latitude(), 1e-9);
    assertEquals(0.0, coordinate.longitude(), 1e-9);
  }

  @Test
  void loggingCallbackCanBeInstalledAndCleared() {
    MapLibre.setLogCallback(record -> true);
    MapLibre.clearLogCallback();
  }
}
