package org.maplibre.nativejni.internal.callback;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;
import org.bytedeco.javacpp.BytePointer;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.resource.ResourceKind;
import org.maplibre.nativejni.resource.ResourceTransformRequest;

// Support invariant for BND-121, BND-123, BND-140, and BND-141: these tests exercise
// Java JNI's callback stub directly to force host failures and in-flight close races.
class ResourceTransformStateTest {
  @Test
  void bnd123CloseWaitsForInflightCallbackBeforeClosingStub() throws Exception {
    var entered = new CountDownLatch(1);
    var release = new CountDownLatch(1);
    var closed = new AtomicBoolean();
    var state =
        new ResourceTransformState(
            request -> {
              entered.countDown();
              assertTrue(await(release));
              return java.util.Optional.of(request.url() + "?rewritten=true");
            });

    try (var url = new BytePointer("https://example.com/style.json");
        var response = new MaplibreNativeC.mln_resource_transform_response()) {
      response.size(response.sizeof());
      response.url(null);

      var callbackThread =
          new Thread(
              () ->
                  state
                      .transform()
                      .callback()
                      .call(null, ResourceKind.STYLE.nativeValue(), url, response));
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
    var state = new AtomicReference<ResourceTransformState>();
    state.set(
        new ResourceTransformState(
            request -> {
              attempted.set(true);
              state.get().close();
              return java.util.Optional.empty();
            }));

    try (var url = new BytePointer("https://example.com/style.json");
        var response = new MaplibreNativeC.mln_resource_transform_response()) {
      response.size(response.sizeof());
      response.url(null);

      var status =
          state
              .get()
              .transform()
              .callback()
              .call(null, ResourceKind.STYLE.nativeValue(), url, response);

      assertEquals(MaplibreNativeC.MLN_STATUS_NATIVE_ERROR, status);
      assertTrue(attempted.get());
      assertFalse(state.get().isClosed());
    } finally {
      state.get().close();
    }
  }

  @Test
  void bnd121InvalidRewriteUrlFallsBackToPassThrough() {
    try (var state = new ResourceTransformState(request -> java.util.Optional.of("bad\0url"));
        var url = new BytePointer("https://example.com/style.json");
        var response = new MaplibreNativeC.mln_resource_transform_response()) {
      response.size(response.sizeof());
      response.url(null);

      var status =
          state.transform().callback().call(null, ResourceKind.STYLE.nativeValue(), url, response);

      assertEquals(MaplibreNativeC.MLN_STATUS_OK, status);
      assertTrue(response.url() == null || response.url().isNull());
    }
  }

  @Test
  void bnd121CallbackExceptionReturnsNativeErrorAndNoRewrite() {
    try (var state =
            new ResourceTransformState(
                request -> {
                  throw new IllegalStateException("boom");
                });
        var url = new BytePointer("https://example.com/style.json");
        var response = new MaplibreNativeC.mln_resource_transform_response()) {
      response.size(response.sizeof());
      response.url(null);

      var status =
          state.transform().callback().call(null, ResourceKind.STYLE.nativeValue(), url, response);

      assertEquals(MaplibreNativeC.MLN_STATUS_NATIVE_ERROR, status);
      assertTrue(response.url() == null || response.url().isNull());
    }
  }

  @Test
  void bnd141RequestCopiesBorrowedUrlAndPreservesRawKind() {
    var captured = new AtomicReference<ResourceTransformRequest>();
    try (var state =
            new ResourceTransformState(
                request -> {
                  captured.set(request);
                  return java.util.Optional.empty();
                });
        var url = new BytePointer("https://example.com/source.json");
        var response = new MaplibreNativeC.mln_resource_transform_response()) {
      response.size(response.sizeof());
      response.url(null);

      var status = state.transform().callback().call(null, 0x7fff, url, response);

      assertEquals(MaplibreNativeC.MLN_STATUS_OK, status);
    }

    assertEquals(0x7fff, captured.get().kind().rawValue());
    assertEquals("https://example.com/source.json", captured.get().url());
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
