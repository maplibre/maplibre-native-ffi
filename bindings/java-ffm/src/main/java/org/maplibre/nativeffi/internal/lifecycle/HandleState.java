package org.maplibre.nativeffi.internal.lifecycle;

import java.lang.foreign.MemorySegment;
import java.lang.ref.Cleaner;
import java.util.LinkedHashMap;
import java.util.Map;
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
  private boolean releasing;
  private int liveChildren;
  private final Map<String, Integer> liveChildCounts = new LinkedHashMap<>();

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
    if (releasing) {
      throw Status.releasing(typeName);
    }
    return handle;
  }

  public synchronized boolean isReleased() {
    return released;
  }

  public long address() {
    return handle.address();
  }

  public synchronized ChildRetention retainChild(String childTypeName) {
    if (released) {
      throw Status.released(typeName);
    }
    if (releasing) {
      throw Status.releasing(typeName);
    }
    liveChildren++;
    liveChildCounts.merge(childTypeName, 1, Integer::sum);
    return new ChildRetention(this, childTypeName);
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
      if (releasing) {
        throw Status.releasing(typeName);
      }
      if (liveChildren > 0) {
        throw Status.liveChildren(typeName, liveChildren, liveChildSummary());
      }
      releasing = true;
    }

    var destroySucceeded = false;
    try {
      Status.check(destroy.destroy(handle));
      destroySucceeded = true;
    } finally {
      if (!destroySucceeded) {
        synchronized (this) {
          releasing = false;
        }
      }
    }

    synchronized (this) {
      released = true;
      releasing = false;
      leakReport.markReleased();
    }
    cleanable.clean();

    afterSuccess.run();
  }

  private synchronized String liveChildSummary() {
    return String.join(
        ", ",
        liveChildCounts.entrySet().stream()
            .map(entry -> entry.getKey() + "=" + entry.getValue())
            .toList());
  }

  private synchronized void releaseChild(String childTypeName) {
    if (liveChildren > 0) {
      liveChildren--;
    }
    liveChildCounts.computeIfPresent(
        childTypeName, (ignored, count) -> count > 1 ? count - 1 : null);
  }

  @FunctionalInterface
  public interface NativeDestroy {
    int destroy(MemorySegment handle);
  }

  public static final class ChildRetention implements AutoCloseable {
    private final HandleState parent;
    private final String childTypeName;

    private boolean released;

    private ChildRetention(HandleState parent, String childTypeName) {
      this.parent = parent;
      this.childTypeName = Objects.requireNonNull(childTypeName, "childTypeName");
    }

    @Override
    public synchronized void close() {
      if (released) {
        return;
      }
      released = true;
      parent.releaseChild(childTypeName);
    }
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
