package org.maplibre.nativejni.internal.callback;

import static org.junit.jupiter.api.Assertions.assertThrows;

import java.util.Optional;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.runtime.RuntimeHandle;
import org.maplibre.nativejni.test.NativeTestSupport;

final class ResourceTransformStateTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibraryOrSkip();
  }

  @Test
  void rejectsZeroNativeStateAddress() {
    assertThrows(IllegalArgumentException.class, () -> new ResourceTransformState(0));
  }

  @Test
  void runtimeOwnsAndClearsTransformState() {
    try (var runtime = RuntimeHandle.create()) {
      runtime.setResourceTransform(request -> Optional.empty());
      runtime.setResourceTransform(request -> Optional.of(request.url() + "?token=1"));
      runtime.clearResourceTransform();
    }
  }
}
