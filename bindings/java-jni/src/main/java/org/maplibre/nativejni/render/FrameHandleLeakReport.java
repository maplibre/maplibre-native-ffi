package org.maplibre.nativejni.render;

import java.lang.ref.Cleaner;
import org.maplibre.nativejni.internal.lifecycle.HandleState;

final class FrameHandleLeakReport implements Runnable {
  private static final Cleaner CLEANER = Cleaner.create();

  private final String typeName;
  private final HandleState.ChildRetention sessionRetention;
  private volatile boolean closed;

  private FrameHandleLeakReport(String typeName, HandleState.ChildRetention sessionRetention) {
    this.typeName = typeName;
    this.sessionRetention = sessionRetention;
  }

  static Registration register(
      Object owner, String typeName, HandleState.ChildRetention sessionRetention) {
    var report = new FrameHandleLeakReport(typeName, sessionRetention);
    return new Registration(report, CLEANER.register(owner, report));
  }

  void markClosed() {
    closed = true;
  }

  @Override
  public void run() {
    try {
      if (!closed) {
        System.err.printf(
            "Leaked %s; close frame handles explicitly on the render session owner thread.%n",
            typeName);
      }
    } finally {
      sessionRetention.close();
    }
  }

  record Registration(FrameHandleLeakReport report, Cleaner.Cleanable cleanable) {}
}
