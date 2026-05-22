package org.maplibre.nativejni;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.junit.jupiter.api.Assumptions.assumeTrue;

import java.nio.file.Files;
import java.nio.file.Path;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.error.InvalidArgumentException;
import org.maplibre.nativejni.error.MaplibreStatus;
import org.maplibre.nativejni.internal.bridge.BaseNative;
import org.maplibre.nativejni.internal.bridge.RuntimeNative;
import org.maplibre.nativejni.internal.loader.NativeLibrary;
import org.maplibre.nativejni.internal.status.Status;
import org.maplibre.nativejni.runtime.NetworkStatus;

class MaplibreTest {
  private static NetworkStatus originalNetworkStatus;

  @BeforeAll
  static void loadNativeLibrary() {
    var libraryPath = System.getProperty(NativeLibrary.LIBRARY_PATH_PROPERTY);
    assumeTrue(
        libraryPath != null && !libraryPath.isBlank(),
        () -> "Set -D" + NativeLibrary.LIBRARY_PATH_PROPERTY + " to a built JNI bridge library");
    assumeTrue(
        Files.isRegularFile(Path.of(libraryPath)), () -> "Missing JNI bridge: " + libraryPath);
    Maplibre.loadNativeLibrary(Path.of(libraryPath));
    originalNetworkStatus = Maplibre.networkStatus();
  }

  @AfterEach
  void restoreNetworkStatus() {
    if (originalNetworkStatus != null) {
      Maplibre.setNetworkStatus(originalNetworkStatus);
    }
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
            BaseNative.mln_supported_render_backend_mask()));
  }

  @Test
  void roundTripsNetworkStatus() {
    Maplibre.setNetworkStatus(NetworkStatus.OFFLINE);
    assertEquals(NetworkStatus.OFFLINE, Maplibre.networkStatus());

    Maplibre.setNetworkStatus(NetworkStatus.ONLINE);
    assertEquals(NetworkStatus.ONLINE, Maplibre.networkStatus());
  }

  @Test
  void convertsNativeStatusAndCapturesDiagnostic() {
    var exception =
        assertThrows(
            InvalidArgumentException.class,
            () -> Status.check(RuntimeNative.mln_network_status_set(999_999)));

    assertEquals(MaplibreStatus.INVALID_ARGUMENT, exception.status());
    assertEquals(MaplibreStatus.INVALID_ARGUMENT.nativeCode(), exception.nativeStatusCode());
    assertFalse(exception.diagnostic().isBlank());
    assertTrue(exception.diagnostic().contains("network status"));
  }
}
