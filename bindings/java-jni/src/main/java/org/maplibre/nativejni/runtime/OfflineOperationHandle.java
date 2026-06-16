package org.maplibre.nativejni.runtime;

import java.lang.ref.Cleaner;
import java.util.Objects;
import org.maplibre.nativejni.error.InvalidStateException;
import org.maplibre.nativejni.error.MaplibreStatus;
import org.maplibre.nativejni.internal.access.InternalAccess;
import org.maplibre.nativejni.internal.lifecycle.HandleState;

/** Owner-thread offline database operation that must be taken or discarded. */
public final class OfflineOperationHandle<T> implements AutoCloseable {
  private static final Cleaner CLEANER = Cleaner.create();

  private final RuntimeHandle runtime;
  private final long id;
  private final OfflineOperationKind kind;
  private final OfflineOperationResultKind resultKind;
  private final HandleState.ChildRetention runtimeRetention;
  private final LeakReport leakReport;
  private final Cleaner.Cleanable cleanable;
  private boolean closed;

  OfflineOperationHandle(
      RuntimeHandle runtime,
      long id,
      OfflineOperationKind kind,
      OfflineOperationResultKind resultKind) {
    this.runtime = Objects.requireNonNull(runtime, "runtime");
    if (id == 0) {
      throw new IllegalArgumentException("offline operation id must not be zero");
    }
    this.id = id;
    this.kind = Objects.requireNonNull(kind, "kind");
    this.resultKind = Objects.requireNonNull(resultKind, "resultKind");
    var retention = runtime.retainChild(InternalAccess.INSTANCE, "OfflineOperationHandle");
    try {
      this.runtimeRetention = retention;
      this.leakReport = new LeakReport(id, kind, resultKind, retention);
      this.cleanable = CLEANER.register(this, leakReport);
    } catch (RuntimeException | Error error) {
      retention.close();
      throw error;
    }
  }

  public synchronized long id() {
    return id;
  }

  public synchronized OfflineOperationKind kind() {
    return kind;
  }

  public synchronized OfflineOperationResultKind resultKind() {
    return resultKind;
  }

  public synchronized boolean isClosed() {
    return closed;
  }

  synchronized long requireLive(RuntimeHandle expectedRuntime) {
    if (closed) {
      throw new InvalidStateException(
          MaplibreStatus.INVALID_STATE.nativeCode(), "OfflineOperationHandle is already closed");
    }
    if (runtime != Objects.requireNonNull(expectedRuntime, "expectedRuntime")) {
      throw new InvalidStateException(
          MaplibreStatus.INVALID_STATE.nativeCode(),
          "OfflineOperationHandle belongs to a different RuntimeHandle");
    }
    return id;
  }

  synchronized long requireLive(
      RuntimeHandle expectedRuntime,
      OfflineOperationKind expectedKind,
      OfflineOperationResultKind expectedResultKind) {
    requireLive(expectedRuntime);
    if (kind != expectedKind || resultKind != expectedResultKind) {
      throw new InvalidStateException(
          MaplibreStatus.INVALID_STATE.nativeCode(),
          "OfflineOperationHandle has kind "
              + kind
              + " and result kind "
              + resultKind
              + ", expected "
              + expectedKind
              + " and "
              + expectedResultKind);
    }
    return id;
  }

  synchronized void markConsumed() {
    closed = true;
    runtimeRetention.close();
    leakReport.markClosed();
    cleanable.clean();
  }

  void reportLeakForTesting() {
    leakReport.run();
  }

  @Override
  public void close() {
    runtime.discardOfflineOperation(this);
  }

  private static final class LeakReport implements Runnable {
    private final long id;
    private final OfflineOperationKind kind;
    private final OfflineOperationResultKind resultKind;
    private final HandleState.ChildRetention runtimeRetention;
    private volatile boolean closed;

    private LeakReport(
        long id,
        OfflineOperationKind kind,
        OfflineOperationResultKind resultKind,
        HandleState.ChildRetention runtimeRetention) {
      this.id = id;
      this.kind = kind;
      this.resultKind = resultKind;
      this.runtimeRetention = runtimeRetention;
    }

    private void markClosed() {
      closed = true;
    }

    @Override
    public void run() {
      try {
        if (!closed) {
          System.err.printf(
              "Leaked OfflineOperationHandle id=%d kind=%s resultKind=%s; take or discard"
                  + " operations explicitly on the runtime owner thread.%n",
              id, kind, resultKind);
        }
      } finally {
        runtimeRetention.close();
      }
    }
  }
}
