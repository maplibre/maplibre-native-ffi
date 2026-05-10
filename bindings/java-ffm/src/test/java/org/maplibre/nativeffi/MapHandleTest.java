package org.maplibre.nativeffi;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.concurrent.atomic.AtomicReference;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.internal.NativeTestSupport;

final class MapHandleTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibrary();
  }

  @Test
  void createsAndClosesMapBeforeRuntime() {
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().setSize(128, 128));
    map.requestRepaint();
    map.close();
    assertTrue(map.isClosed());
    runtime.close();
  }

  @Test
  void runtimeCloseFailsWhileMapIsLive() {
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().setSize(128, 128));
    try {
      var error = assertThrows(InvalidStateException.class, runtime::close);
      assertEquals(MapLibreStatus.INVALID_STATE, error.status());
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void releasedMapRejectsLaterMethodsBeforeNativeDispatch() {
    var runtime = RuntimeHandle.create();
    try {
      var map = MapHandle.create(runtime, new MapOptions().setSize(128, 128));
      map.close();
      var error = assertThrows(InvalidStateException.class, () -> map.setStyleJson("{}"));
      assertTrue(error.diagnostic().contains("MapHandle"));
    } finally {
      runtime.close();
    }
  }

  @Test
  void projectionHelpersCloseIndependently() {
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().setSize(128, 128));
    try {
      var projection = map.createProjection();
      var point = projection.pixelForLatLng(new LatLng(0, 0));
      var coordinate = projection.latLngForPixel(point);
      assertEquals(0.0, coordinate.latitude(), 1e-6);
      assertEquals(0.0, coordinate.longitude(), 1e-6);
      projection.close();
      assertThrows(InvalidStateException.class, () -> projection.pixelForLatLng(new LatLng(0, 0)));
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void wrongThreadMapCallMapsToWrongThreadException() throws Exception {
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().setSize(128, 128));
    try {
      assertWrongThread(runOnOtherThread(map::requestRepaint));
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void wrongThreadProjectionCallMapsToWrongThreadException() throws Exception {
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().setSize(128, 128));
    var projection = map.createProjection();
    try {
      assertWrongThread(runOnOtherThread(() -> projection.pixelForLatLng(new LatLng(0, 0))));
    } finally {
      projection.close();
      map.close();
      runtime.close();
    }
  }

  private static void assertWrongThread(Throwable thrown) {
    assertTrue(thrown instanceof WrongThreadException, () -> String.valueOf(thrown));
    var error = (WrongThreadException) thrown;
    assertEquals(MapLibreStatus.WRONG_THREAD, error.status());
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

  @FunctionalInterface
  private interface ThrowingRunnable {
    void run() throws Exception;
  }
}
