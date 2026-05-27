package org.maplibre.nativejni.resource;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

import java.util.concurrent.atomic.AtomicInteger;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.error.InvalidStateException;

class ResourceRequestHandleTest {
  @Test
  void passThroughDecisionLetsNativeReleaseHandle() {
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
  void providerExceptionMarksNativeReleaseAndClosesHandle() {
    var releaseCount = new AtomicInteger();
    var handle = new ResourceRequestHandle(0x1234, ignored -> releaseCount.incrementAndGet());

    assertEquals(-1, handle.finishProviderException());
    handle.close();

    assertEquals(0, releaseCount.get());
    assertThrows(InvalidStateException.class, handle::isCancelled);
  }

  @Test
  void handleDecisionReleasesExactlyOnceOnClose() {
    var releaseCount = new AtomicInteger();
    var handle = new ResourceRequestHandle(0x1234, ignored -> releaseCount.incrementAndGet());

    assertEquals(
        ResourceProviderDecision.HANDLE.nativeValue(),
        handle.finishProviderDecision(ResourceProviderDecision.HANDLE));
    handle.close();
    handle.close();

    assertEquals(1, releaseCount.get());
  }
}
