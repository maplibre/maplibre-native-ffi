package org.maplibre.nativejni.runtime;

import static org.junit.jupiter.api.Assertions.assertThrows;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.resource.ResourceProviderDecision;
import org.maplibre.nativejni.test.NativeTestSupport;

final class ResourceProviderStateTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibraryOrSkip();
  }

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
