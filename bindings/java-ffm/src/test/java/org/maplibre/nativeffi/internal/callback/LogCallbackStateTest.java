package org.maplibre.nativeffi.internal.callback;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.Maplibre;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_log_callback;
import org.maplibre.nativeffi.internal.memory.MemoryUtil;
import org.maplibre.nativeffi.log.LogRecord;
import org.maplibre.nativeffi.log.LogSeverity;
import org.maplibre.nativeffi.test.NativeTestSupport;

final class LogCallbackStateTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibrary();
  }

  @AfterEach
  void clearCallback() {
    Maplibre.clearLogCallback();
  }

  @Test
  void callbackStateSurvivesUntilCleared() {
    var seen = new AtomicReference<LogRecord>();
    Maplibre.setLogCallback(
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
    Maplibre.setLogCallback(
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
