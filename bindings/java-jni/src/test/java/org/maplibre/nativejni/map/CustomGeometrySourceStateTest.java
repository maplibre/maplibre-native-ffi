package org.maplibre.nativejni.map;

import static org.junit.jupiter.api.Assertions.assertDoesNotThrow;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.maplibre.nativejni.geo.CanonicalTileId;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.style.CustomGeometrySourceCallback;
import org.maplibre.nativejni.style.CustomGeometrySourceOptions;
import org.maplibre.nativejni.test.NativeLibraryExtension;

@ExtendWith(NativeLibraryExtension.class)
// Support invariant for BND-121, BND-123, and BND-124: direct callback-stub tests
// make custom-geometry teardown and host exception behavior deterministic.
class CustomGeometrySourceStateTest {
  @Test
  void bnd123AndBnd124CloseWaitsForInflightTileCallbackBeforeClosingStubs() throws Exception {
    var entered = new CountDownLatch(1);
    var release = new CountDownLatch(1);
    var closed = new AtomicBoolean();
    var state =
        new CustomGeometrySourceState(
            new CustomGeometrySourceOptions(
                tileId -> {
                  entered.countDown();
                  assertTrue(await(release));
                }));

    try (var tileId = new MaplibreNativeC.mln_canonical_tile_id()) {
      tileId.z(1).x(2).y(3);
      var callbackThread = new Thread(() -> state.descriptor().fetch_tile().call(null, tileId));
      callbackThread.start();
      assertTrue(await(entered));

      var closeThread =
          new Thread(
              () -> {
                state.close();
                closed.set(true);
              });
      closeThread.start();

      Thread.sleep(25);
      assertFalse(closed.get());
      release.countDown();
      callbackThread.join();
      closeThread.join();

      assertTrue(closed.get());
    }
  }

  @Test
  void bnd123CloseFromCurrentCallbackFailsWithoutDeadlock() {
    var attempted = new AtomicBoolean();
    var state = new AtomicReference<CustomGeometrySourceState>();
    state.set(
        new CustomGeometrySourceState(
            new CustomGeometrySourceOptions(
                tileId -> {
                  attempted.set(true);
                  state.get().close();
                })));

    try (var tileId = new MaplibreNativeC.mln_canonical_tile_id()) {
      tileId.z(1).x(2).y(3);
      state.get().descriptor().fetch_tile().call(null, tileId);

      assertTrue(attempted.get());
      assertFalse(state.get().isClosedForTesting());
    } finally {
      state.get().close();
    }
  }

  @Test
  void bnd121CallbackExceptionsDoNotEscapeNativeTileCallbacks() {
    var fetchCalls = new AtomicInteger();
    var cancelCalls = new AtomicInteger();
    var state =
        new CustomGeometrySourceState(
            new CustomGeometrySourceOptions(
                new CustomGeometrySourceCallback() {
                  @Override
                  public void fetchTile(CanonicalTileId tileId) {
                    fetchCalls.incrementAndGet();
                    throw new IllegalStateException("fetch failed");
                  }

                  @Override
                  public void cancelTile(CanonicalTileId tileId) {
                    cancelCalls.incrementAndGet();
                    throw new IllegalStateException("cancel failed");
                  }
                }));

    try (var tileId = new MaplibreNativeC.mln_canonical_tile_id()) {
      tileId.z(1).x(2).y(3);

      assertDoesNotThrow(() -> state.descriptor().fetch_tile().call(null, tileId));
      assertDoesNotThrow(() -> state.descriptor().cancel_tile().call(null, tileId));

      assertEquals(1, fetchCalls.get());
      assertEquals(1, cancelCalls.get());
    } finally {
      state.close();
    }
  }

  private static boolean await(CountDownLatch latch) {
    try {
      return latch.await(5, TimeUnit.SECONDS);
    } catch (InterruptedException exception) {
      Thread.currentThread().interrupt();
      return false;
    }
  }
}
