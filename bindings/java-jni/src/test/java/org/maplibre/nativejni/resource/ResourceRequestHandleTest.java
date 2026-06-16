package org.maplibre.nativejni.resource;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.error.InvalidArgumentException;
import org.maplibre.nativejni.error.InvalidStateException;
import org.maplibre.nativejni.error.NativeErrorException;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.internal.status.Status;

// Resource-provider request tests use fake native completers and releasers to make BND-146
// through BND-153 deterministic without adding extra public resource workflows.
class ResourceRequestHandleTest {
  @Test
  void bnd023AndBnd142PassThroughDecisionLetsNativeReleaseHandle() {
    var releaseCount = new AtomicInteger();
    var handle = new ResourceRequestHandle(0x1234, ignored -> releaseCount.incrementAndGet());

    assertEquals(
        ResourceProviderDecision.PASS_THROUGH.nativeValue(),
        handle.finishProviderDecision(ResourceProviderDecision.PASS_THROUGH));
    handle.close();

    assertEquals(0, releaseCount.get());
    assertThrows(InvalidStateException.class, handle::isCancelled);
  }

  @Test
  void bnd121ProviderExceptionMarksNativeReleaseAndClosesHandle() {
    var releaseCount = new AtomicInteger();
    var handle = new ResourceRequestHandle(0x1234, ignored -> releaseCount.incrementAndGet());

    assertEquals(-1, handle.finishProviderException());
    handle.close();

    assertEquals(0, releaseCount.get());
    assertThrows(InvalidStateException.class, handle::isCancelled);
  }

  @Test
  void bnd147HandleDecisionReleasesExactlyOnceOnClose() {
    var releaseCount = new AtomicInteger();
    var handle = new ResourceRequestHandle(0x1234, ignored -> releaseCount.incrementAndGet());

    assertEquals(
        ResourceProviderDecision.HANDLE.nativeValue(),
        handle.finishProviderDecision(ResourceProviderDecision.HANDLE));
    handle.close();
    handle.close();

    assertEquals(1, releaseCount.get());
  }

  @Test
  void bnd023AndBnd146AndBnd147CompletionReleasesOwnedHandleAndRejectsLaterUse() {
    var completeCount = new AtomicInteger();
    var releaseCount = new AtomicInteger();
    var handle =
        new ResourceRequestHandle(
            0x1234,
            (address, response) -> {
              completeCount.incrementAndGet();
              return MaplibreNativeC.MLN_STATUS_OK;
            },
            ignored -> releaseCount.incrementAndGet());

    assertEquals(
        ResourceProviderDecision.HANDLE.nativeValue(),
        handle.finishProviderDecision(ResourceProviderDecision.HANDLE));
    handle.complete(ResourceResponse.noContent());
    handle.close();

    assertEquals(1, completeCount.get());
    assertEquals(1, releaseCount.get());
    assertThrows(InvalidStateException.class, () -> handle.complete(ResourceResponse.noContent()));
    assertThrows(InvalidStateException.class, handle::isCancelled);
  }

  @Test
  void bnd150InlineCompletionForcesHandledDecisionEvenWhenCallbackLaterPassesThrough() {
    var completeCount = new AtomicInteger();
    var releaseCount = new AtomicInteger();
    var handle =
        new ResourceRequestHandle(
            0x1234,
            (address, response) -> {
              completeCount.incrementAndGet();
              return MaplibreNativeC.MLN_STATUS_OK;
            },
            ignored -> releaseCount.incrementAndGet());

    handle.complete(ResourceResponse.noContent());

    assertEquals(
        ResourceProviderDecision.HANDLE.nativeValue(),
        handle.finishProviderDecision(ResourceProviderDecision.PASS_THROUGH));
    handle.close();

    assertEquals(1, completeCount.get());
    assertEquals(1, releaseCount.get());
    assertThrows(InvalidStateException.class, () -> handle.complete(ResourceResponse.noContent()));
  }

  @Test
  void bnd152CompletionThatReachesNativeIsTerminalEvenWhenNativeReturnsError() {
    var completeCount = new AtomicInteger();
    var releaseCount = new AtomicInteger();
    var handle =
        new ResourceRequestHandle(
            0x1234,
            (address, response) -> {
              completeCount.incrementAndGet();
              return MaplibreNativeC.MLN_STATUS_INVALID_STATE;
            },
            ignored -> releaseCount.incrementAndGet());

    assertEquals(
        ResourceProviderDecision.HANDLE.nativeValue(),
        handle.finishProviderDecision(ResourceProviderDecision.HANDLE));
    assertThrows(InvalidStateException.class, () -> handle.complete(ResourceResponse.noContent()));
    handle.close();

    assertEquals(1, completeCount.get());
    assertEquals(1, releaseCount.get());
    assertThrows(InvalidStateException.class, () -> handle.complete(ResourceResponse.noContent()));
    assertThrows(InvalidStateException.class, handle::isCancelled);
  }

  @Test
  void bnd026CompletionFailureReportsOriginalDiagnosticAfterReleaseCleanup() {
    var releaseCount = new AtomicInteger();
    var handle =
        new ResourceRequestHandle(
            0x1234,
            (address, response) -> {
              JavaCppSupport.setThreadDiagnostic("completion failed");
              return MaplibreNativeC.MLN_STATUS_INVALID_STATE;
            },
            ignored -> {
              releaseCount.incrementAndGet();
              JavaCppSupport.setThreadDiagnostic("release cleanup failed");
            });

    assertEquals(
        ResourceProviderDecision.HANDLE.nativeValue(),
        handle.finishProviderDecision(ResourceProviderDecision.HANDLE));
    var error =
        assertThrows(
            InvalidStateException.class, () -> handle.complete(ResourceResponse.noContent()));

    assertEquals("completion failed", error.diagnostic());
    assertEquals(1, releaseCount.get());
    assertThrows(InvalidStateException.class, () -> handle.complete(ResourceResponse.noContent()));
    var later =
        assertThrows(
            NativeErrorException.class,
            () -> Status.check(MaplibreNativeC.MLN_STATUS_NATIVE_ERROR));
    assertFalse(later.diagnostic().contains("release cleanup failed"));
  }

  @Test
  void bnd024ResponseStringValidationFailsBeforeNativeCompletionAndCanRetry() {
    var completeCount = new AtomicInteger();
    var releaseCount = new AtomicInteger();
    var handle =
        new ResourceRequestHandle(
            0x1234,
            (address, response) -> {
              completeCount.incrementAndGet();
              return MaplibreNativeC.MLN_STATUS_OK;
            },
            ignored -> releaseCount.incrementAndGet());

    assertEquals(
        ResourceProviderDecision.HANDLE.nativeValue(),
        handle.finishProviderDecision(ResourceProviderDecision.HANDLE));
    assertThrows(
        InvalidArgumentException.class,
        () -> handle.complete(ResourceResponse.error(ResourceErrorReason.OTHER, "bad\0message")));
    assertThrows(
        InvalidArgumentException.class,
        () -> handle.complete(ResourceResponse.noContent().etag("bad\0etag")));

    handle.complete(ResourceResponse.noContent());

    assertEquals(1, completeCount.get());
    assertEquals(1, releaseCount.get());
  }

  @Test
  void bnd068UnknownResourceErrorReasonIsRejectedBeforeNativeCompletionAndCanRetry() {
    var completeCount = new AtomicInteger();
    var releaseCount = new AtomicInteger();
    var handle =
        new ResourceRequestHandle(
            0x1234,
            (address, response) -> {
              completeCount.incrementAndGet();
              return MaplibreNativeC.MLN_STATUS_OK;
            },
            ignored -> releaseCount.incrementAndGet());

    assertEquals(
        ResourceProviderDecision.HANDLE.nativeValue(),
        handle.finishProviderDecision(ResourceProviderDecision.HANDLE));
    assertThrows(
        IllegalArgumentException.class,
        () ->
            handle.complete(
                ResourceResponse.error(ResourceErrorReason.fromNative(999), "unknown")));

    handle.complete(ResourceResponse.error(ResourceErrorReason.OTHER, "other"));

    assertEquals(1, completeCount.get());
    assertEquals(1, releaseCount.get());
  }

  @Test
  void bnd153CloseWaitsForInflightCompletionBeforeNativeRelease() throws Exception {
    var enteredCompletion = new CountDownLatch(1);
    var releaseCompletion = new CountDownLatch(1);
    var closeReturned = new CountDownLatch(1);
    var completeFailure = new AtomicReference<Throwable>();
    var releaseCount = new AtomicInteger();
    var handle =
        new ResourceRequestHandle(
            0x1234,
            (address, response) -> {
              enteredCompletion.countDown();
              assertTrue(await(releaseCompletion));
              return MaplibreNativeC.MLN_STATUS_OK;
            },
            ignored -> releaseCount.incrementAndGet());

    assertEquals(
        ResourceProviderDecision.HANDLE.nativeValue(),
        handle.finishProviderDecision(ResourceProviderDecision.HANDLE));

    var completeThread =
        new Thread(
            () -> {
              try {
                handle.complete(ResourceResponse.noContent());
              } catch (Throwable throwable) {
                completeFailure.set(throwable);
              }
            });
    completeThread.start();
    assertTrue(await(enteredCompletion));

    var closeThread =
        new Thread(
            () -> {
              handle.close();
              closeReturned.countDown();
            });
    closeThread.start();

    Thread.sleep(25);
    assertEquals(0, releaseCount.get());
    assertFalse(closeReturned.await(25, TimeUnit.MILLISECONDS));

    releaseCompletion.countDown();
    completeThread.join();
    closeThread.join();

    if (completeFailure.get() != null) {
      throw new AssertionError("completion failed", completeFailure.get());
    }
    assertEquals(1, releaseCount.get());
    assertTrue(closeReturned.await(5, TimeUnit.SECONDS));
    assertThrows(InvalidStateException.class, () -> handle.complete(ResourceResponse.noContent()));
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
