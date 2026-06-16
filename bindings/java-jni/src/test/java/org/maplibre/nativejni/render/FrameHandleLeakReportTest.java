package org.maplibre.nativejni.render;

import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.internal.lifecycle.HandleState;

// Support invariant for BND-044 and BND-048: frame cleanup is non-deterministic in Java,
// so leaked thread-affine frames are reported through the documented leak channel.
class FrameHandleLeakReportTest {
  @Test
  void bnd044AndBnd048ReportsLeaksAndSuppressesClosedFrames() {
    var parent = new HandleState("ParentHandle", 0x1234);
    var registration =
        FrameHandleLeakReport.register(
            new Object(), "TestFrameHandle", parent.retainChild("TestFrameHandle"));

    var leaked = captureStderr(registration.report()::run);
    assertTrue(leaked.contains("Leaked TestFrameHandle"));
    parent.closeOnce(address -> 0);

    var closedParent = new HandleState("ClosedParentHandle", 0x5678);
    registration =
        FrameHandleLeakReport.register(
            new Object(), "TestFrameHandle", closedParent.retainChild("ClosedTestFrameHandle"));
    registration.report().markClosed();
    var closed = captureStderr(registration.report()::run);
    assertTrue(closed.isEmpty());
    closedParent.closeOnce(address -> 0);
    registration.cleanable().clean();
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
