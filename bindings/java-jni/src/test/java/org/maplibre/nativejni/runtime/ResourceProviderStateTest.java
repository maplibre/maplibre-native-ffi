package org.maplibre.nativejni.runtime;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;
import org.bytedeco.javacpp.BytePointer;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.resource.ResourceKind;
import org.maplibre.nativejni.resource.ResourceProviderDecision;

// Support invariant for BND-121, BND-123, and BND-142 through BND-145: these tests
// call the Java JNI provider stub directly to make host failures and in-flight close races
// deterministic.
final class ResourceProviderStateTest {
  @Test
  void bnd142RejectsNullCallback() {
    assertThrows(NullPointerException.class, () -> new ResourceProviderState(null));
  }

  @Test
  void bnd142RuntimeOwnsProviderState() {
    try (var runtime = RuntimeHandle.create()) {
      runtime.setResourceProvider((request, handle) -> ResourceProviderDecision.PASS_THROUGH);
      runtime.setResourceProvider((request, handle) -> ResourceProviderDecision.PASS_THROUGH);
    }
  }

  @Test
  void bnd123AndBnd145CloseWaitsForInflightCallbackBeforeClosingStub() throws Exception {
    var entered = new CountDownLatch(1);
    var release = new CountDownLatch(1);
    var closed = new AtomicBoolean();
    var callbackResult = new AtomicReference<Integer>();
    var callbackFailure = new AtomicReference<Throwable>();
    var state =
        new ResourceProviderState(
            (request, handle) -> {
              entered.countDown();
              assertTrue(await(release));
              return ResourceProviderDecision.PASS_THROUGH;
            });

    try (var url = new BytePointer("https://example.com/style.json");
        var request = new MaplibreNativeC.mln_resource_request();
        var handle = JavaCppSupport.resourceRequestHandle(0x1234)) {
      request.size(request.sizeof());
      request.url(url);
      request.kind(ResourceKind.STYLE.nativeValue());

      var callbackThread =
          new Thread(
              () -> {
                try {
                  callbackResult.set(state.provider().callback().call(null, request, handle));
                } catch (Throwable failure) {
                  callbackFailure.set(failure);
                }
              });
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
      callbackThread.join(5_000);
      closeThread.join(5_000);

      assertFalse(callbackThread.isAlive());
      assertFalse(closeThread.isAlive());
      if (callbackFailure.get() != null) {
        throw new AssertionError("callback thread failed", callbackFailure.get());
      }
      assertEquals(ResourceProviderDecision.PASS_THROUGH.nativeValue(), callbackResult.get());
      assertTrue(closed.get());
    }
  }

  @Test
  void bnd123CloseFromCurrentCallbackFailsWithoutDeadlock() {
    var attempted = new AtomicBoolean();
    var state = new AtomicReference<ResourceProviderState>();
    state.set(
        new ResourceProviderState(
            (request, handle) -> {
              attempted.set(true);
              state.get().close();
              return ResourceProviderDecision.PASS_THROUGH;
            }));

    try (var url = new BytePointer("https://example.com/style.json");
        var request = new MaplibreNativeC.mln_resource_request();
        var handle = JavaCppSupport.resourceRequestHandle(0x1234)) {
      request.size(request.sizeof());
      request.url(url);
      request.kind(ResourceKind.STYLE.nativeValue());

      assertEquals(-1, state.get().provider().callback().call(null, request, handle));
      assertTrue(attempted.get());
      assertFalse(state.get().isClosed());
    } finally {
      state.get().close();
    }
  }

  @Test
  void bnd121CallbackExceptionReturnsProviderFailureDecision() {
    var state =
        new ResourceProviderState(
            (request, handle) -> {
              throw new IllegalStateException("boom");
            });

    try (var url = new BytePointer("https://example.com/style.json");
        var request = new MaplibreNativeC.mln_resource_request();
        var handle = JavaCppSupport.resourceRequestHandle(0x1234)) {
      request.size(request.sizeof());
      request.url(url);
      request.kind(ResourceKind.STYLE.nativeValue());

      assertEquals(-1, state.provider().callback().call(null, request, handle));
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
