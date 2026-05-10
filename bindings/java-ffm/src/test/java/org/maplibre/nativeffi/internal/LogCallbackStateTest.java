package org.maplibre.nativeffi.internal;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.LogRecord;
import org.maplibre.nativeffi.LogSeverity;
import org.maplibre.nativeffi.MapLibre;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_log_callback;

final class LogCallbackStateTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibrary();
  }

  @AfterEach
  void clearCallback() {
    MapLibre.clearLogCallback();
  }

  @Test
  void callbackStateSurvivesUntilCleared() {
    var seen = new AtomicReference<LogRecord>();
    MapLibre.setLogCallback(
        record -> {
          seen.set(record);
          return true;
        });

    var state = LogCallbackState.currentForTesting();
    assertNotNull(state);
    try (var arena = Arena.ofConfined()) {
      var message = MemoryUtil.allocateCString(arena, "hello");
      var consumed =
          mln_log_callback.invoke(
              state.stubForTesting(),
              MemorySegment.NULL,
              MapLibreNativeC.MLN_LOG_SEVERITY_INFO(),
              MapLibreNativeC.MLN_LOG_EVENT_GENERAL(),
              7,
              message);
      assertEquals(1, consumed);
      assertEquals("hello", seen.get().message());
      assertEquals(LogSeverity.INFO, seen.get().severity());
    }
  }

  @Test
  void callbackExceptionsDoNotUnwindIntoNativeCode() {
    MapLibre.setLogCallback(
        record -> {
          throw new AssertionError("boom");
        });

    var state = LogCallbackState.currentForTesting();
    assertNotNull(state);
    try (var arena = Arena.ofConfined()) {
      var message = MemoryUtil.allocateCString(arena, "ignored");
      var consumed =
          mln_log_callback.invoke(
              state.stubForTesting(),
              MemorySegment.NULL,
              MapLibreNativeC.MLN_LOG_SEVERITY_ERROR(),
              MapLibreNativeC.MLN_LOG_EVENT_GENERAL(),
              0,
              message);
      assertEquals(0, consumed);
    }
  }
}
