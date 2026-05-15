package org.maplibre.nativeffi.runtime;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertInstanceOf;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.nio.charset.StandardCharsets;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.Maplibre;
import org.maplibre.nativeffi.error.InvalidStateException;
import org.maplibre.nativeffi.error.MaplibreStatus;
import org.maplibre.nativeffi.error.NativeErrorException;
import org.maplibre.nativeffi.error.WrongThreadException;
import org.maplibre.nativeffi.map.MapHandle;
import org.maplibre.nativeffi.map.MapOptions;
import org.maplibre.nativeffi.render.OwnedTextureDescriptor;
import org.maplibre.nativeffi.render.RenderSessionHandle;
import org.maplibre.nativeffi.render.RenderTargetExtent;
import org.maplibre.nativeffi.resource.ResourceKind;
import org.maplibre.nativeffi.resource.ResourceProviderDecision;
import org.maplibre.nativeffi.resource.ResourceRequestHandle;
import org.maplibre.nativeffi.resource.ResourceResponse;
import org.maplibre.nativeffi.test.NativeTestSupport;

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

  @AfterEach
  void restoreProcessState() {
    Maplibre.clearLogCallback();
    Maplibre.restoreDefaultAsyncLogSeverities();
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
    assertEquals(MaplibreStatus.INVALID_STATE, error.status());
    assertTrue(error.diagnostic().contains("RuntimeHandle"));
  }

  @Test
  void setsResourceCallbacksBeforeMapsAreCreated() {
    var runtime = RuntimeHandle.create();
    try {
      runtime.setResourceTransform(request -> java.util.Optional.empty());
      runtime.clearResourceTransform();
      runtime.setResourceProvider((request, handle) -> ResourceProviderDecision.PASS_THROUGH);
    } finally {
      runtime.close();
    }
  }

  @Test
  void resourceTransformUpdatesAndClearsAfterMapCreation() {
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(128, 128));
    try {
      runtime.setResourceTransform(request -> java.util.Optional.empty());
      runtime.clearResourceTransform();
    } finally {
      map.close();
      runtime.close();
    }
  }

  @Test
  void resourceProviderRejectsInstallAfterMapCreation() {
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(128, 128));
    try {
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
      var map = MapHandle.create(runtime, new MapOptions().size(128, 128));
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
  void resourceProviderCanCompleteHandledRequestAfterCallbackReturns() throws Exception {
    var runtime = RuntimeHandle.create();
    var handledRequest = new AtomicReference<ResourceRequestHandle>();
    var callbackExited = new CountDownLatch(1);
    try {
      runtime.setResourceProvider(
          (request, handle) -> {
            if (!"custom://async-style.json".equals(request.url())) {
              return ResourceProviderDecision.PASS_THROUGH;
            }
            try {
              handledRequest.set(handle);
              return ResourceProviderDecision.HANDLE;
            } finally {
              callbackExited.countDown();
            }
          });
      var map = MapHandle.create(runtime, new MapOptions().size(128, 128));
      try {
        map.setStyleUrl("custom://async-style.json");
        assertTrue(callbackExited.await(5, TimeUnit.SECONDS));
        var handle = handledRequest.get();
        assertFalse(handle.isCancelled());
        handle.complete(ResourceResponse.ok(STYLE_JSON.getBytes(StandardCharsets.UTF_8)));
        assertThrows(InvalidStateException.class, handle::isCancelled);
        assertThrows(
            InvalidStateException.class, () -> handle.complete(ResourceResponse.noContent()));
        handle.close();
        assertTrue(waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED));
      } finally {
        map.close();
      }
    } finally {
      runtime.close();
    }
  }

  @Test
  void runtimeEventsCopyPayloadAndMessageBeforeNextPoll() throws Exception {
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(64, 64));
    RenderSessionHandle session = null;
    try {
      var failure = assertThrows(NativeErrorException.class, () -> map.setStyleJson("{"));
      assertFalse(failure.diagnostic().isBlank());
      var failedEvent = waitForMapEventRecord(runtime, map, RuntimeEventType.MAP_LOADING_FAILED);
      assertFalse(failedEvent.message().isBlank());

      session =
          map.attachOwnedTexture(
              new OwnedTextureDescriptor().extent(new RenderTargetExtent(64, 64, 1.0)));
      map.setStyleJson(STYLE_JSON);
      waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE);
      session.renderUpdate();
      var frameEvent =
          waitForMapEventRecord(runtime, map, RuntimeEventType.MAP_RENDER_FRAME_FINISHED);
      var frame = assertInstanceOf(RuntimeEventPayload.RenderFrame.class, frameEvent.payload());
      var frameCount = frame.stats().frameCount();

      runtime.pollEvent();
      runtime.runOnce();
      runtime.pollEvent();

      assertFalse(failedEvent.message().isBlank());
      assertEquals(frameCount, frame.stats().frameCount());
    } finally {
      if (session != null) {
        session.close();
      }
      map.close();
      runtime.close();
    }
  }

  @Test
  void wrongThreadRuntimeCallMapsToWrongThreadException() throws Exception {
    var runtime = RuntimeHandle.create();
    try {
      assertWrongThread(runOnOtherThread(runtime::runOnce));
    } finally {
      runtime.close();
    }
  }

  @Test
  void wrongThreadRuntimeCloseLeavesHandleLive() throws Exception {
    var runtime = RuntimeHandle.create();
    try {
      assertWrongThread(runOnOtherThread(runtime::close));
      assertFalse(runtime.isClosed());
    } finally {
      runtime.close();
    }
  }

  private static boolean waitForMapEvent(
      RuntimeHandle runtime, MapHandle map, RuntimeEventType eventType)
      throws InterruptedException {
    return waitForMapEventRecord(runtime, map, eventType) != null;
  }

  private static RuntimeEvent waitForMapEventRecord(
      RuntimeHandle runtime, MapHandle map, RuntimeEventType eventType)
      throws InterruptedException {
    for (var attempts = 0; attempts < 1_000; attempts++) {
      runtime.runOnce();
      while (true) {
        var event = runtime.pollEvent();
        if (event.isEmpty()) {
          break;
        }
        var value = event.get();
        if (value.type() == eventType && value.mapSource().orElse(null) == map) {
          return value;
        }
      }
      Thread.sleep(1);
    }
    return null;
  }

  private static void assertWrongThread(Throwable thrown) {
    assertTrue(thrown instanceof WrongThreadException, () -> String.valueOf(thrown));
    var error = (WrongThreadException) thrown;
    assertEquals(MaplibreStatus.WRONG_THREAD, error.status());
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
