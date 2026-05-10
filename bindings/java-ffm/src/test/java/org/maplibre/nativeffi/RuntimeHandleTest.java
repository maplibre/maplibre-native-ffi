package org.maplibre.nativeffi;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.concurrent.atomic.AtomicReference;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.internal.NativeTestSupport;

final class RuntimeHandleTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibrary();
  }

  @Test
  void createsRunsPollsAndClosesRuntime() {
    var runtime = RuntimeHandle.create();
    runtime.runOnce();
    assertTrue(runtime.pollEvent().isEmpty());
    runtime.close();
    assertTrue(runtime.isClosed());
    runtime.close();
  }

  @Test
  void releasedRuntimeRejectsLaterMethodsBeforeNativeDispatch() {
    var runtime = RuntimeHandle.create();
    runtime.close();
    var error = assertThrows(InvalidStateException.class, runtime::runOnce);
    assertEquals(MapLibreStatus.INVALID_STATE, error.status());
    assertTrue(error.diagnostic().contains("RuntimeHandle"));
  }

  @Test
  void wrongThreadRuntimeCallMapsToWrongThreadException() throws Exception {
    var runtime = RuntimeHandle.create();
    try {
      var thrown = new AtomicReference<Throwable>();
      var thread =
          new Thread(
              () -> {
                try {
                  runtime.runOnce();
                } catch (Throwable error) {
                  thrown.set(error);
                }
              });
      thread.start();
      thread.join();

      assertTrue(thrown.get() instanceof WrongThreadException, () -> String.valueOf(thrown.get()));
      var error = (WrongThreadException) thrown.get();
      assertEquals(MapLibreStatus.WRONG_THREAD, error.status());
      assertFalse(error.diagnostic().isBlank());
    } finally {
      runtime.close();
    }
  }
}
