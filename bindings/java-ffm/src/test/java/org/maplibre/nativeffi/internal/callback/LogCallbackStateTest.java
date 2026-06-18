package org.maplibre.nativeffi.internal.callback;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertInstanceOf;
import static org.junit.jupiter.api.Assertions.assertSame;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.Maplibre;
import org.maplibre.nativeffi.error.InvalidStateException;
import org.maplibre.nativeffi.internal.c.mln_log_callback;
import org.maplibre.nativeffi.internal.convert.NativeValues;
import org.maplibre.nativeffi.internal.memory.MemoryUtil;
import org.maplibre.nativeffi.log.LogEvent;
import org.maplibre.nativeffi.log.LogSeverity;
import org.maplibre.nativeffi.test.NativeTestSupport;

final class LogCallbackStateTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibrary();
  }

  @Test
  void clearWaitsForActiveLogUpcalls() throws Exception {
    var entered = new CountDownLatch(1);
    var release = new CountDownLatch(1);
    var executor = Executors.newFixedThreadPool(2);
    try (var arena = Arena.ofShared()) {
      Maplibre.setLogCallback(
          record -> {
            entered.countDown();
            await(release);
            return true;
          });
      var state = LogCallbackState.currentForTesting();
      var invoke =
          executor.submit(
              () ->
                  mln_log_callback.invoke(
                      state.stubForTesting(),
                      MemorySegment.NULL,
                      NativeValues.nativeValue(LogSeverity.INFO),
                      NativeValues.nativeValue(LogEvent.GENERAL),
                      0,
                      MemoryUtil.allocateCString(arena, "message")));
      assertTrue(entered.await(5, TimeUnit.SECONDS));

      var clear = executor.submit(Maplibre::clearLogCallback);
      assertThrows(TimeoutException.class, () -> clear.get(100, TimeUnit.MILLISECONDS));

      release.countDown();
      assertEquals(1, invoke.get(5, TimeUnit.SECONDS));
      clear.get(5, TimeUnit.SECONDS);
    } finally {
      release.countDown();
      executor.shutdownNow();
      Maplibre.clearLogCallback();
      Maplibre.restoreDefaultAsyncLogSeverities();
    }
  }

  @Test
  void clearFromActiveLogUpcallFailsWithoutClearingCurrentCallback() throws Exception {
    var executor = Executors.newSingleThreadExecutor();
    var callbackFailure = new AtomicReference<Throwable>();
    try (var arena = Arena.ofShared()) {
      Maplibre.setLogCallback(
          record -> {
            try {
              Maplibre.clearLogCallback();
            } catch (Throwable error) {
              callbackFailure.set(error);
            }
            return true;
          });
      var state = LogCallbackState.currentForTesting();
      var invoke =
          executor.submit(
              () ->
                  mln_log_callback.invoke(
                      state.stubForTesting(),
                      MemorySegment.NULL,
                      NativeValues.nativeValue(LogSeverity.INFO),
                      NativeValues.nativeValue(LogEvent.GENERAL),
                      0,
                      MemoryUtil.allocateCString(arena, "message")));

      assertEquals(1, invoke.get(5, TimeUnit.SECONDS));
      assertInstanceOf(InvalidStateException.class, callbackFailure.get());
      assertSame(state, LogCallbackState.currentForTesting());
    } finally {
      executor.shutdownNow();
      Maplibre.clearLogCallback();
      Maplibre.restoreDefaultAsyncLogSeverities();
    }
  }

  @Test
  void callbackExceptionsDoNotUnwindIntoNativeCode() {
    Maplibre.setLogCallback(
        record -> {
          throw new AssertionError("boom");
        });

    var state = LogCallbackState.currentForTesting();
    try (var arena = Arena.ofConfined()) {
      var consumed =
          mln_log_callback.invoke(
              state.stubForTesting(),
              MemorySegment.NULL,
              NativeValues.nativeValue(LogSeverity.ERROR),
              NativeValues.nativeValue(LogEvent.GENERAL),
              0,
              MemoryUtil.allocateCString(arena, "ignored"));

      assertEquals(0, consumed);
    } finally {
      Maplibre.clearLogCallback();
      Maplibre.restoreDefaultAsyncLogSeverities();
    }
  }

  private static void await(CountDownLatch latch) {
    try {
      if (!latch.await(5, TimeUnit.SECONDS)) {
        throw new AssertionError("timed out waiting for latch");
      }
    } catch (InterruptedException error) {
      Thread.currentThread().interrupt();
      throw new AssertionError(error);
    }
  }
}
