package org.maplibre.nativejni.internal.lifecycle;

import static org.junit.jupiter.api.Assertions.assertDoesNotThrow;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.error.InvalidStateException;

// Support invariant for BND-040, BND-041, BND-042, and BND-046: HandleState is the
// shared Java JNI state machine behind public handle ownership, release, and close races.
class HandleStateTest {
  @Test
  void bnd044ReportsLeaksAndSuppressesReleasedHandles() {
    var state = new HandleState("TestHandle", 0x1234);
    var leaked = captureStderr(state::reportLeakForTesting);
    assertTrue(leaked.contains("Leaked TestHandle native handle 0x1234"));
    state.closeOnce(address -> 0);
    assertThrows(InvalidStateException.class, state::requireLiveAddress);
    var released = captureStderr(state::reportLeakForTesting);
    assertTrue(released.isEmpty());
  }

  @Test
  void bnd040RejectsNullNativeHandles() {
    assertThrows(IllegalArgumentException.class, () -> new HandleState("TestHandle", 0));
  }

  @Test
  void bnd041LeavesHandleLiveWhenNativeDestroyFails() {
    var state = new HandleState("TestHandle", 0x5678);
    assertThrows(InvalidStateException.class, () -> state.closeOnce(address -> -2));
    assertDoesNotThrow(state::requireLiveAddress);
  }

  @Test
  void bnd042RejectsParentCloseWhileChildrenAreLive() {
    var state = new HandleState("ParentHandle", 0x6789);
    var child = state.retainChild("ChildHandle");

    var error = assertThrows(InvalidStateException.class, () -> state.closeOnce(address -> 0));
    assertTrue(error.diagnostic().contains("live child handle"));
    assertDoesNotThrow(state::requireLiveAddress);

    child.close();
    assertDoesNotThrow(() -> state.closeOnce(address -> 0));
  }

  @Test
  void bnd040AndBnd046RejectsCallsDuringCloseAndDestroysOnce() throws Exception {
    var state = new HandleState("TestHandle", 0x789a);
    var entered = new CountDownLatch(1);
    var release = new CountDownLatch(1);
    var destroyCalls = new AtomicInteger();
    var closeThread =
        new Thread(
            () ->
                state.closeOnce(
                    address -> {
                      destroyCalls.incrementAndGet();
                      entered.countDown();
                      assertTrue(await(release));
                      return 0;
                    }));

    closeThread.start();
    assertTrue(await(entered));

    var liveError = assertThrows(InvalidStateException.class, state::requireLiveAddress);
    assertTrue(liveError.diagnostic().contains("closing"));
    var closeError = assertThrows(InvalidStateException.class, () -> state.closeOnce(address -> 0));
    assertTrue(closeError.diagnostic().contains("closing"));

    release.countDown();
    closeThread.join();

    assertTrue(state.isReleased());
    state.closeOnce(
        address -> {
          destroyCalls.incrementAndGet();
          return 0;
        });
    assertEquals(1, destroyCalls.get());
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

  private static boolean await(CountDownLatch latch) {
    try {
      return latch.await(5, TimeUnit.SECONDS);
    } catch (InterruptedException exception) {
      Thread.currentThread().interrupt();
      return false;
    }
  }
}
