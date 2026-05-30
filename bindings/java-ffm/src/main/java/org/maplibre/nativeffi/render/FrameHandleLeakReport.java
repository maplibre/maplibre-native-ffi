package org.maplibre.nativeffi.render;

import java.lang.ref.Cleaner;

final class FrameHandleLeakReport implements Runnable {
  private static final Cleaner CLEANER = Cleaner.create();

  private final String typeName;
  private volatile boolean closed;

  private FrameHandleLeakReport(String typeName) {
    this.typeName = typeName;
  }

  static Registration register(Object owner, String typeName) {
    var report = new FrameHandleLeakReport(typeName);
    return new Registration(report, CLEANER.register(owner, report));
  }

  void markClosed() {
    closed = true;
  }

  @Override
  public void run() {
    if (!closed) {
      System.err.printf(
          "Leaked %s; close frame handles explicitly on the render session owner thread.%n",
          typeName);
    }
  }

  record Registration(FrameHandleLeakReport report, Cleaner.Cleanable cleanable) {}
}
