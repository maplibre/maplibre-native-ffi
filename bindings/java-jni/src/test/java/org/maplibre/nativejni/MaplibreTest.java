package org.maplibre.nativejni;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertInstanceOf;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.Set;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.error.InvalidArgumentException;
import org.maplibre.nativejni.error.InvalidStateException;
import org.maplibre.nativejni.error.MaplibreException;
import org.maplibre.nativejni.error.MaplibreStatus;
import org.maplibre.nativejni.error.NativeErrorException;
import org.maplibre.nativejni.error.UnsupportedFeatureException;
import org.maplibre.nativejni.error.WrongThreadException;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.internal.status.Status;
import org.maplibre.nativejni.log.LogSeverity;
import org.maplibre.nativejni.render.OpenGLContextProviderSupport;
import org.maplibre.nativejni.render.RenderBackendSupport;
import org.maplibre.nativejni.runtime.NetworkStatus;

class MaplibreTest {
  private static NetworkStatus originalNetworkStatus;

  @BeforeAll
  static void captureOriginalNetworkStatus() {
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
  void bnd001ReadsCAbiVersion() {
    assertTrue(Maplibre.cVersion() >= 0);
  }

  @Test
  void bnd160ReadsSupportedRenderBackends() {
    assertEquals(
        RenderBackendSupport.fromMask(MaplibreNativeC.mln_supported_render_backend_mask()),
        Maplibre.supportedRenderBackends());
  }

  @Test
  void bnd160ReadsSupportedOpenGLContextProviders() {
    assertEquals(
        OpenGLContextProviderSupport.fromMask(
            MaplibreNativeC.mln_opengl_supported_context_provider_mask()),
        Maplibre.supportedOpenGLContextProviders());
  }

  @Test
  void bnd060AndBnd062RoundTripsNetworkStatus() {
    Maplibre.setNetworkStatus(NetworkStatus.OFFLINE);
    assertEquals(NetworkStatus.OFFLINE, Maplibre.networkStatus());
    assertEquals(NetworkStatus.OFFLINE.nativeValue(), Maplibre.networkStatus().rawValue());

    Maplibre.setNetworkStatus(NetworkStatus.ONLINE);
    assertEquals(NetworkStatus.ONLINE, Maplibre.networkStatus());
    assertEquals(NetworkStatus.ONLINE.nativeValue(), Maplibre.networkStatus().rawValue());
    var unknown = NetworkStatus.fromNative(999);
    assertEquals(999, unknown.rawValue());
    assertThrows(IllegalArgumentException.class, unknown::nativeValue);
  }

  @Test
  void bnd103RoundTripsProjectedMeters() {
    var coordinate = new LatLng(37.7749, -122.4194);

    var meters = Maplibre.projectedMetersForLatLng(coordinate);
    var roundTrip = Maplibre.latLngForProjectedMeters(meters);

    assertEquals(coordinate.latitude(), roundTrip.latitude(), 1.0e-9);
    assertEquals(coordinate.longitude(), roundTrip.longitude(), 1.0e-9);
  }

  @Test
  void bnd120InstallsAndClearsLogCallback() {
    Maplibre.setLogCallback(record -> true);
    Maplibre.clearLogCallback();
  }

  @Test
  void bnd120ConfiguresAsyncLogSeverities() {
    Maplibre.setAsyncLogSeverities(Set.of(LogSeverity.ERROR));
    Maplibre.restoreDefaultAsyncLogSeverities();
  }

  @Test
  void bnd020NativeStatusMapsInvalidArgumentAndCapturesDiagnostic() {
    var exception =
        assertThrows(
            InvalidArgumentException.class,
            () -> Status.check(MaplibreNativeC.mln_network_status_set(999_999)));

    assertEquals(MaplibreStatus.INVALID_ARGUMENT, exception.status());
    assertEquals(MaplibreStatus.INVALID_ARGUMENT.nativeCode(), exception.nativeStatusCode());
    assertFalse(exception.diagnostic().isBlank());
    assertTrue(exception.diagnostic().contains("network status"));
  }

  @Test
  void bnd022NativeDiagnosticIsCopiedBeforeLaterNativeFailureChangesThreadLocalDiagnostic() {
    var first =
        assertThrows(
            InvalidArgumentException.class,
            () -> Status.check(MaplibreNativeC.mln_network_status_set(999_999)));
    var firstDiagnostic = first.diagnostic();

    var second =
        assertThrows(
            InvalidArgumentException.class,
            () -> Status.check(MaplibreNativeC.mln_runtime_run_once(null)));

    assertEquals(firstDiagnostic, first.diagnostic());
    assertTrue(first.diagnostic().contains("network status"));
    assertFalse(second.diagnostic().isBlank());
    assertFalse(second.diagnostic().equals(firstDiagnostic));
  }

  @Test
  void bnd020NativeErrorStatusMapsToPublicNativeErrorException() {
    var exception =
        assertThrows(
            NativeErrorException.class,
            () -> Status.check(MaplibreNativeC.MLN_STATUS_NATIVE_ERROR));

    assertEquals(MaplibreStatus.NATIVE_ERROR, exception.status());
    assertEquals(MaplibreStatus.NATIVE_ERROR.nativeCode(), exception.nativeStatusCode());
  }

  @Test
  void bnd020NativeStatusCategoriesMapToPublicExceptionTypes() {
    assertStatusException(
        MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT,
        MaplibreStatus.INVALID_ARGUMENT,
        InvalidArgumentException.class);
    assertStatusException(
        MaplibreNativeC.MLN_STATUS_INVALID_STATE,
        MaplibreStatus.INVALID_STATE,
        InvalidStateException.class);
    assertStatusException(
        MaplibreNativeC.MLN_STATUS_WRONG_THREAD,
        MaplibreStatus.WRONG_THREAD,
        WrongThreadException.class);
    assertStatusException(
        MaplibreNativeC.MLN_STATUS_UNSUPPORTED,
        MaplibreStatus.UNSUPPORTED,
        UnsupportedFeatureException.class);
    assertStatusException(
        MaplibreNativeC.MLN_STATUS_NATIVE_ERROR,
        MaplibreStatus.NATIVE_ERROR,
        NativeErrorException.class);
  }

  @Test
  void bnd021UnknownNativeStatusPreservesRawStatusCode() {
    var exception = assertThrows(MaplibreException.class, () -> Status.check(123_456));

    assertEquals(123_456, exception.status().nativeCode());
    assertEquals(123_456, exception.nativeStatusCode());
  }

  private static void assertStatusException(
      int nativeStatus, MaplibreStatus expectedStatus, Class<? extends MaplibreException> type) {
    var exception = assertThrows(MaplibreException.class, () -> Status.check(nativeStatus));
    assertInstanceOf(type, exception);
    assertEquals(expectedStatus, exception.status());
    assertEquals(nativeStatus, exception.nativeStatusCode());
  }
}
