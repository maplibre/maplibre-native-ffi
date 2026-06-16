package org.maplibre.nativeffi.resource;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.lang.foreign.MemorySegment;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.error.InvalidArgumentException;
import org.maplibre.nativeffi.error.InvalidStateException;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.convert.NativeValues;

final class ResourceRequestHandleTest {
  @Test
  void handleDecisionCanRetainHandleAfterCallback() {
    var releases = new AtomicInteger();
    var handle =
        new ResourceRequestHandle(
            MemorySegment.ofAddress(0x1234), ignored -> releases.incrementAndGet());

    assertEquals(
        NativeValues.nativeValue(ResourceProviderDecision.HANDLE),
        handle.finishProviderDecision(ResourceProviderDecision.HANDLE));
    assertEquals(0, releases.get());

    handle.close();
    assertEquals(1, releases.get());
  }

  @Test
  void completionThatReachesNativeIsTerminalWhenNativeReturnsError() {
    var completions = new AtomicInteger();
    var releases = new AtomicInteger();
    var handle =
        new ResourceRequestHandle(
            MemorySegment.ofAddress(0x1234),
            ignored -> releases.incrementAndGet(),
            (ignoredHandle, ignoredResponse) -> {
              completions.incrementAndGet();
              return MapLibreNativeC.MLN_STATUS_INVALID_STATE();
            });
    assertEquals(
        NativeValues.nativeValue(ResourceProviderDecision.HANDLE),
        handle.finishProviderDecision(ResourceProviderDecision.HANDLE));

    assertThrows(InvalidStateException.class, () -> handle.complete(ResourceResponse.noContent()));

    assertEquals(1, completions.get());
    assertEquals(1, releases.get());
    assertThrows(InvalidStateException.class, () -> handle.complete(ResourceResponse.noContent()));
    assertEquals(1, completions.get());
    handle.close();
    assertEquals(1, releases.get());
  }

  @Test
  void completionPreCallFailureIsTerminalAfterProviderDecision() {
    var completions = new AtomicInteger();
    var releases = new AtomicInteger();
    var handle =
        new ResourceRequestHandle(
            MemorySegment.ofAddress(0x1234),
            ignored -> releases.incrementAndGet(),
            (ignoredHandle, ignoredResponse) -> {
              completions.incrementAndGet();
              return MapLibreNativeC.MLN_STATUS_OK();
            });
    assertEquals(
        NativeValues.nativeValue(ResourceProviderDecision.HANDLE),
        handle.finishProviderDecision(ResourceProviderDecision.HANDLE));

    var unknownReason = NativeValues.resourceErrorReason(999_996);
    assertEquals(999_996, unknownReason.rawValue());
    assertFalse(ResourceErrorReason.NONE.equals(unknownReason));
    assertThrows(
        InvalidArgumentException.class,
        () -> handle.complete(ResourceResponse.error(unknownReason, "unknown")));

    assertEquals(0, completions.get());
    assertEquals(1, releases.get());
    assertThrows(InvalidStateException.class, () -> handle.complete(ResourceResponse.noContent()));
    assertEquals(0, completions.get());
    handle.close();
    assertEquals(1, releases.get());
  }

  @Test
  void passThroughDecisionClosesStaleHandleWithoutProviderRelease() {
    var releases = new AtomicInteger();
    var cancellations = new AtomicInteger();
    var handle =
        new ResourceRequestHandle(
            MemorySegment.ofAddress(0x1234),
            ignored -> releases.incrementAndGet(),
            (ignoredHandle, ignoredResponse) -> MapLibreNativeC.MLN_STATUS_OK(),
            ignored -> {
              cancellations.incrementAndGet();
              return false;
            });

    assertEquals(
        NativeValues.nativeValue(ResourceProviderDecision.PASS_THROUGH),
        handle.finishProviderDecision(ResourceProviderDecision.PASS_THROUGH));

    assertThrows(InvalidStateException.class, () -> handle.complete(ResourceResponse.noContent()));
    assertThrows(InvalidStateException.class, handle::isCancelled);
    handle.close();
    assertEquals(0, releases.get());
    assertEquals(0, cancellations.get());
  }

  @Test
  void closeWaitsForInFlightCancellationCheckBeforeReleasing() throws Exception {
    var entered = new CountDownLatch(1);
    var release = new CountDownLatch(1);
    var releases = new AtomicInteger();
    var handle =
        new ResourceRequestHandle(
            MemorySegment.ofAddress(0x1234),
            ignored -> releases.incrementAndGet(),
            (ignoredHandle, ignoredResponse) -> MapLibreNativeC.MLN_STATUS_OK(),
            ignored -> {
              entered.countDown();
              await(release);
              return false;
            });
    assertEquals(
        NativeValues.nativeValue(ResourceProviderDecision.HANDLE),
        handle.finishProviderDecision(ResourceProviderDecision.HANDLE));

    var executor = Executors.newFixedThreadPool(2);
    try {
      var cancellation = executor.submit(handle::isCancelled);
      assertTrue(entered.await(5, TimeUnit.SECONDS));

      var close = executor.submit(handle::close);
      assertThrows(TimeoutException.class, () -> close.get(100, TimeUnit.MILLISECONDS));
      assertEquals(0, releases.get());

      release.countDown();
      assertFalse(cancellation.get(5, TimeUnit.SECONDS));
      close.get(5, TimeUnit.SECONDS);
      assertEquals(1, releases.get());
      assertThrows(InvalidStateException.class, handle::isCancelled);
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
