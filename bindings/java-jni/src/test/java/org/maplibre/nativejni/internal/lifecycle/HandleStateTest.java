package org.maplibre.nativejni.internal.lifecycle;

import static org.junit.jupiter.api.Assertions.assertDoesNotThrow;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.error.InvalidStateException;
import org.maplibre.nativejni.test.NativeTestSupport;

class HandleStateTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibraryOrSkip();
  }

  @Test
  void reportsLeaksAndSuppressesReleasedHandles() {
    var state = new HandleState("TestHandle", 0x1234);

    var leaked = captureStderr(state::reportLeakForTesting);
    assertTrue(leaked.contains("Leaked TestHandle native handle 0x1234"));

    state.closeOnce(address -> 0);
    assertThrows(InvalidStateException.class, state::requireLiveAddress);

    var released = captureStderr(state::reportLeakForTesting);
    assertTrue(released.isEmpty());
  }

  @Test
  void rejectsNullNativeHandles() {
    assertThrows(IllegalArgumentException.class, () -> new HandleState("TestHandle", 0));
  }

  @Test
  void leavesHandleLiveWhenNativeDestroyFails() {
    var state = new HandleState("TestHandle", 0x5678);

    assertThrows(InvalidStateException.class, () -> state.closeOnce(address -> -2));
    assertDoesNotThrow(state::requireLiveAddress);
  }

  private static String captureStderr(Runnable runnable) {
    var previous = System.err;
    var bytes = new ByteArrayOutputStream();
    try {
      System.setErr(new PrintStream(bytes, true, StandardCharsets.UTF_8));
      runnable.run();
    } finally {
      System.setErr(previous);
    }
    return bytes.toString(StandardCharsets.UTF_8);
  }
}
