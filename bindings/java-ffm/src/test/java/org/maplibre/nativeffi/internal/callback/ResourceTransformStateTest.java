package org.maplibre.nativeffi.internal.callback;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.util.Optional;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_resource_transform;
import org.maplibre.nativeffi.internal.c.mln_resource_transform_callback;
import org.maplibre.nativeffi.internal.c.mln_resource_transform_response;
import org.maplibre.nativeffi.internal.convert.NativeValues;
import org.maplibre.nativeffi.internal.memory.MemoryUtil;
import org.maplibre.nativeffi.resource.ResourceKind;
import org.maplibre.nativeffi.test.NativeTestSupport;

final class ResourceTransformStateTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibrary();
  }

  @Test
  void clearsStaleResponseUrlWhenKeepingOriginalUrl() {
    try (var state = new ResourceTransformState(request -> Optional.empty());
        var arena = Arena.ofConfined()) {
      var response = mln_resource_transform_response.allocate(arena);
      mln_resource_transform_response.url(
          response, MemoryUtil.allocateCString(arena, "https://stale.example.test/style.json"));

      var status =
          mln_resource_transform_callback.invoke(
              mln_resource_transform.callback(state.descriptor()),
              MemorySegment.NULL,
              NativeValues.nativeValue(ResourceKind.STYLE),
              MemoryUtil.allocateCString(arena, "https://example.test/style.json"),
              response);

      assertEquals(MapLibreNativeC.MLN_STATUS_OK(), status);
      assertEquals(MemorySegment.NULL, mln_resource_transform_response.url(response));
    }
  }

  @Test
  void callbackExceptionsBecomeNativeErrorStatus() {
    try (var state =
            new ResourceTransformState(
                request -> {
                  throw new AssertionError("boom");
                });
        var arena = Arena.ofConfined()) {
      var response = mln_resource_transform_response.allocate(arena);
      var status =
          mln_resource_transform_callback.invoke(
              mln_resource_transform.callback(state.descriptor()),
              MemorySegment.NULL,
              NativeValues.nativeValue(ResourceKind.STYLE),
              MemoryUtil.allocateCString(arena, "https://example.test/style.json"),
              response);

      assertEquals(MapLibreNativeC.MLN_STATUS_NATIVE_ERROR(), status);
    }
  }

  @Test
  void nullCallbackResponseBecomesNativeErrorStatus() {
    try (var state = new ResourceTransformState(request -> null);
        var arena = Arena.ofConfined()) {
      var response = mln_resource_transform_response.allocate(arena);
      var status =
          mln_resource_transform_callback.invoke(
              mln_resource_transform.callback(state.descriptor()),
              MemorySegment.NULL,
              NativeValues.nativeValue(ResourceKind.STYLE),
              MemoryUtil.allocateCString(arena, "https://example.test/style.json"),
              response);

      assertEquals(MapLibreNativeC.MLN_STATUS_NATIVE_ERROR(), status);
    }
  }

  @Test
  void closeWaitsForActiveResourceTransformUpcalls() throws Exception {
    var entered = new CountDownLatch(1);
    var release = new CountDownLatch(1);
    var state =
        new ResourceTransformState(
            request -> {
              entered.countDown();
              await(release);
              return Optional.empty();
            });
    var executor = Executors.newFixedThreadPool(2);
    try (var arena = Arena.ofShared()) {
      var response = mln_resource_transform_response.allocate(arena);
      var invoke =
          executor.submit(
              () ->
                  mln_resource_transform_callback.invoke(
                      mln_resource_transform.callback(state.descriptor()),
                      MemorySegment.NULL,
                      NativeValues.nativeValue(ResourceKind.STYLE),
                      MemoryUtil.allocateCString(arena, "https://example.test/style.json"),
                      response));
      assertTrue(entered.await(5, TimeUnit.SECONDS));

      var close = executor.submit(state::close);
      assertThrows(TimeoutException.class, () -> close.get(100, TimeUnit.MILLISECONDS));

      release.countDown();
      assertEquals(MapLibreNativeC.MLN_STATUS_OK(), invoke.get(5, TimeUnit.SECONDS));
      close.get(5, TimeUnit.SECONDS);
    } finally {
      release.countDown();
      executor.shutdownNow();
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
