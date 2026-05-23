package org.maplibre.nativejni.internal.lifecycle;

import java.lang.ref.Cleaner;
import java.util.Objects;
import org.maplibre.nativejni.internal.status.Status;

/** Shared released-state bookkeeping for JNI native handles. */
public final class HandleState {
  private static final Cleaner CLEANER = Cleaner.create();

  private final String typeName;
  private final long address;
  private final LeakReport leakReport;
  private final Cleaner.Cleanable cleanable;

  @SuppressWarnings("unused")
  private final Object[] parents;

  private boolean released;

  public HandleState(String typeName, long address, Object... parents) {
    this.typeName = Objects.requireNonNull(typeName, "typeName");
    if (address == 0) {
      throw new IllegalArgumentException(typeName + " native handle is null");
    }
    this.address = address;
    this.parents = parents == null ? new Object[0] : parents.clone();
    this.leakReport = new LeakReport(typeName, address);
    this.cleanable = CLEANER.register(this, leakReport);
  }

  public synchronized long requireLiveAddress() {
    if (released) {
      throw Status.released(typeName);
    }
    return address;
  }

  public synchronized boolean isReleased() {
    return released;
  }

  public long address() {
    return address;
  }

  public void closeOnce(NativeDestroy destroy) {
    closeOnce(destroy, () -> {});
  }

  void reportLeakForTesting() {
    leakReport.run();
  }

  public void closeOnce(NativeDestroy destroy, Runnable afterSuccess) {
    Objects.requireNonNull(destroy, "destroy");
    Objects.requireNonNull(afterSuccess, "afterSuccess");

    synchronized (this) {
      if (released) {
        return;
      }
      Status.check(destroy.destroy(address));
      released = true;
      leakReport.markReleased();
      cleanable.clean();
    }

    afterSuccess.run();
  }

  @FunctionalInterface
  public interface NativeDestroy {
    int destroy(long address);
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
