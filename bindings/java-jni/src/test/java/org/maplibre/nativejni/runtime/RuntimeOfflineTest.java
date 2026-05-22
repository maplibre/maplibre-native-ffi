package org.maplibre.nativejni.runtime;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.test.NativeTestSupport;

final class RuntimeOfflineTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibraryOrSkip();
  }

  @Test
  void ambientCacheOperationsExposeTypedHandles() {
    try (var runtime = RuntimeHandle.create()) {
      var operation = runtime.startAmbientCacheOperation(AmbientCacheOperation.CLEAR);

      assertFalse(operation.isClosed());
      assertEquals(OfflineOperationKind.AMBIENT_CACHE, operation.kind());
      assertEquals(OfflineOperationResultKind.NONE, operation.resultKind());
      assertTrue(operation.id() != 0);

      runtime.discardOfflineOperation(operation);
      assertTrue(operation.isClosed());
    }
  }
}
