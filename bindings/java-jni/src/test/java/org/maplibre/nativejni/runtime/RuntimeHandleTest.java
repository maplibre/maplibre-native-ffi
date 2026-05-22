package org.maplibre.nativejni.runtime;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.error.InvalidStateException;
import org.maplibre.nativejni.test.NativeTestSupport;

class RuntimeHandleTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibraryOrSkip();
  }

  @Test
  void createRunOnceAndCloseRuntime() {
    var runtime = RuntimeHandle.create();

    assertFalse(runtime.isClosed());
    runtime.runOnce();
    runtime.close();
    assertTrue(runtime.isClosed());

    runtime.close();
    assertTrue(runtime.isClosed());
    assertThrows(InvalidStateException.class, runtime::runOnce);
  }

  @Test
  void pollEventReturnsEmptyWhenNativeQueueIsEmpty() {
    try (var runtime = RuntimeHandle.create()) {
      runtime.runOnce();
      assertTrue(runtime.pollEvent().isEmpty());
    }
  }
}
