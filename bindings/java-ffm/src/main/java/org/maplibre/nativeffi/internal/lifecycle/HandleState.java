package org.maplibre.nativeffi.internal.lifecycle;

import java.lang.foreign.MemorySegment;
import java.lang.ref.Cleaner;
import java.util.Objects;
import org.maplibre.nativeffi.internal.memory.MemoryUtil;
import org.maplibre.nativeffi.internal.status.Status;

/** Shared released-state bookkeeping for native handles. */
public final class HandleState {
  private static final Cleaner CLEANER = Cleaner.create();

  private final String typeName;
  private final MemorySegment handle;
  private final LeakReport leakReport;
  private final Cleaner.Cleanable cleanable;

  @SuppressWarnings("unused")
  private final Object[] parents;

  private boolean released;

  public HandleState(String typeName, MemorySegment handle, Object... parents) {
    this.typeName = Objects.requireNonNull(typeName, "typeName");
    this.handle = Objects.requireNonNull(handle, "handle");
    if (MemoryUtil.isNull(handle)) {
      throw new IllegalArgumentException(typeName + " native handle is null");
    }
    this.parents = parents == null ? new Object[0] : parents.clone();
    this.leakReport = new LeakReport(typeName, handle.address());
    this.cleanable = CLEANER.register(this, leakReport);
  }

  public synchronized MemorySegment requireLive() {
    if (released) {
      throw Status.released(typeName);
    }
    return handle;
  }

  public synchronized boolean isReleased() {
    return released;
  }

  public long address() {
    return handle.address();
  }

  public void closeOnce(NativeDestroy destroy) {
    closeOnce(destroy, () -> {});
  }

  public void closeOnce(NativeDestroy destroy, Runnable afterSuccess) {
    Objects.requireNonNull(destroy, "destroy");
    Objects.requireNonNull(afterSuccess, "afterSuccess");

    synchronized (this) {
      if (released) {
        return;
      }
      Status.check(destroy.destroy(handle));
      released = true;
      leakReport.markReleased();
      cleanable.clean();
    }

    afterSuccess.run();
  }

  @FunctionalInterface
  public interface NativeDestroy {
    int destroy(MemorySegment handle);
  }

  private static final class LeakReport implements Runnable {
    private final String typeName;
    private final long address;
    private volatile boolean released;

    private LeakReport(String typeName, long address) {
      this.typeName = typeName;
      this.address = address;
    }

    private void markReleased() {
      released = true;
    }

    @Override
    public void run() {
      if (!released) {
        System.err.printf(
            "Leaked %s native handle 0x%x; close handles explicitly on their owner thread.%n",
            typeName, address);
      }
    }
  }
}
