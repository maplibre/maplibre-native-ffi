package org.maplibre.nativejni.runtime;

import static org.junit.jupiter.api.Assertions.assertThrows;

import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.resource.ResourceProviderDecision;

final class ResourceProviderStateTest {
  @Test
  void rejectsNullCallback() {
    assertThrows(NullPointerException.class, () -> new ResourceProviderState(null));
  }

  @Test
  void runtimeOwnsProviderState() {
    try (var runtime = RuntimeHandle.create()) {
      runtime.setResourceProvider((request, handle) -> ResourceProviderDecision.PASS_THROUGH);
      runtime.setResourceProvider((request, handle) -> ResourceProviderDecision.PASS_THROUGH);
    }
  }
}
