package org.maplibre.nativeffi;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertThrows;

import java.util.Set;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.error.InvalidArgumentException;
import org.maplibre.nativeffi.geo.LatLng;
import org.maplibre.nativeffi.internal.convert.NativeValues;
import org.maplibre.nativeffi.log.LogSeverity;
import org.maplibre.nativeffi.render.RenderBackend;
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
    var backends = Maplibre.supportedRenderBackends();
    assertNotNull(backends);
    var providers = Maplibre.supportedOpenGLContextProviders();
    assertNotNull(providers);
    if (backends.contains(RenderBackend.OPENGL)) {
      assertFalse(providers.isEmpty());
    } else {
      assertEquals(Set.of(), providers);
    }
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
  void unknownNetworkStatusPreservesRawValueAndIsRejectedAsInput() {
    var unknown = NativeValues.networkStatus(999_999);

    assertEquals(999_999, unknown.rawValue());
    assertNotEquals(NetworkStatus.ONLINE, unknown);
    assertNotEquals(NetworkStatus.OFFLINE, unknown);
    assertEquals(unknown, NativeValues.networkStatus(999_999));
    assertThrows(InvalidArgumentException.class, () -> Maplibre.setNetworkStatus(unknown));
  }

  @Test
  void configuresAsyncLogSeverities() {
    Maplibre.setAsyncLogSeverities(Set.of());
    Maplibre.setAsyncLogSeverities(Set.of(LogSeverity.INFO, LogSeverity.WARNING));
    assertThrows(
        IllegalArgumentException.class,
        () -> Maplibre.setAsyncLogSeverities(Set.of(LogSeverity.fromNative(999_999))));
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
