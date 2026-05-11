package org.maplibre.nativeffi.resource;

import static org.junit.jupiter.api.Assertions.assertEquals;

import java.lang.foreign.MemorySegment;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.jupiter.api.Test;

final class ResourceRequestHandleTest {
  @Test
  void handleDecisionCanRetainHandleAfterCallback() {
    var releases = new AtomicInteger();
    var handle =
        new ResourceRequestHandle(
            MemorySegment.ofAddress(0x1234), ignored -> releases.incrementAndGet());

    assertEquals(
        ResourceProviderDecision.HANDLE.nativeValue(),
        handle.finishProviderDecision(ResourceProviderDecision.HANDLE));
    assertEquals(0, releases.get());

    handle.close();
    assertEquals(1, releases.get());
  }
}
