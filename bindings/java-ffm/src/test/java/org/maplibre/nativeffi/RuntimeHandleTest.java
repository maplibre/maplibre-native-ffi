package org.maplibre.nativeffi;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.nio.charset.StandardCharsets;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.internal.NativeTestSupport;

final class RuntimeHandleTest {
  private static final String STYLE_JSON =
      """
      {
        "version": 8,
        "name": "java-ffm-test",
        "sources": {},
        "layers": [
          {"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}}
        ]
      }
      """;

  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibrary();
  }

  @Test
  void createsRunsPollsAndClosesRuntime() {
    var runtime = RuntimeHandle.create();
    runtime.runOnce();
    assertTrue(runtime.pollEvent().isEmpty());
    runtime.close();
    assertTrue(runtime.isClosed());
    runtime.close();
  }

  @Test
  void releasedRuntimeRejectsLaterMethodsBeforeNativeDispatch() {
    var runtime = RuntimeHandle.create();
    runtime.close();
    var error = assertThrows(InvalidStateException.class, runtime::runOnce);
    assertEquals(MapLibreStatus.INVALID_STATE, error.status());
    assertTrue(error.diagnostic().contains("RuntimeHandle"));
  }

  @Test
  void setsResourceCallbacksBeforeMapsAreCreated() {
    var runtime = RuntimeHandle.create();
    try {
      runtime.setResourceTransform(request -> java.util.Optional.empty());
      runtime.setResourceProvider((request, handle) -> ResourceProviderDecision.PASS_THROUGH);
    } finally {
      runtime.close();
    }
  }

  @Test
  void resourceCallbacksRejectInstallAfterMapCreation() {
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().setSize(128, 128));
    try {
      assertThrows(
          InvalidStateException.class,
          () -> runtime.setResourceTransform(request -> java.util.Optional.empty()));
      assertThrows(
          InvalidStateException.class,
          () ->
              runtime.setResourceProvider(
                  (request, handle) -> ResourceProviderDecision.PASS_THROUGH));
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void resourceProviderCompletesStyleRequestAtCAbiBoundary() throws Exception {
    var runtime = RuntimeHandle.create();
    var providerCalls = new AtomicInteger();
    var callbackError = new AtomicReference<Throwable>();
    try {
      runtime.setResourceProvider(
          (request, handle) -> {
            try {
              if (!"custom://style.json".equals(request.url())) {
                return ResourceProviderDecision.PASS_THROUGH;
              }
              providerCalls.incrementAndGet();
              assertEquals(ResourceKind.STYLE, request.kind());
              handle.complete(ResourceResponse.ok(STYLE_JSON.getBytes(StandardCharsets.UTF_8)));
              assertThrows(
                  InvalidStateException.class, () -> handle.complete(ResourceResponse.noContent()));
              assertThrows(InvalidStateException.class, handle::isCancelled);
              return ResourceProviderDecision.PASS_THROUGH;
            } catch (Throwable error) {
              callbackError.set(error);
              throw error;
            }
          });
      var map = MapHandle.create(runtime, new MapOptions().setSize(128, 128));
      try {
        map.setStyleUrl("custom://style.json");
        assertTrue(
            waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED),
            () -> "callback error: " + callbackError.get());
        assertEquals(1, providerCalls.get());
        assertNull(callbackError.get(), () -> String.valueOf(callbackError.get()));
      } finally {
        map.close();
      }
    } finally {
      runtime.close();
    }
  }

  @Test
  void wrongThreadRuntimeCallMapsToWrongThreadException() throws Exception {
    var runtime = RuntimeHandle.create();
    try {
      var thrown = new AtomicReference<Throwable>();
      var thread =
          new Thread(
              () -> {
                try {
                  runtime.runOnce();
                } catch (Throwable error) {
                  thrown.set(error);
                }
              });
      thread.start();
      thread.join();

      assertTrue(thrown.get() instanceof WrongThreadException, () -> String.valueOf(thrown.get()));
      var error = (WrongThreadException) thrown.get();
      assertEquals(MapLibreStatus.WRONG_THREAD, error.status());
      assertFalse(error.diagnostic().isBlank());
    } finally {
      runtime.close();
    }
  }

  private static boolean waitForMapEvent(
      RuntimeHandle runtime, MapHandle map, RuntimeEventType eventType)
      throws InterruptedException {
    for (var attempts = 0; attempts < 1_000; attempts++) {
      runtime.runOnce();
      while (true) {
        var event = runtime.pollEvent();
        if (event.isEmpty()) {
          break;
        }
        if (event.get().type() == eventType && event.get().mapSource().orElse(null) == map) {
          return true;
        }
      }
      Thread.sleep(1);
    }
    return false;
  }
}
