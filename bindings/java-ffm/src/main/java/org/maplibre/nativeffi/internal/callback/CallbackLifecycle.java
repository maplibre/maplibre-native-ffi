package org.maplibre.nativeffi.internal.callback;

import java.util.Objects;
import java.util.Optional;
import java.util.concurrent.TimeUnit;
import org.maplibre.nativeffi.internal.status.Status;

/** Tracks callback upcalls so native stubs are released only after active calls return. */
public final class CallbackLifecycle {
  private static final int MAX_DEFERRED_CLOSE_ATTEMPTS = 1_000;
  private static final System.Logger LOGGER = System.getLogger(CallbackLifecycle.class.getName());

  private final ThreadLocal<Integer> currentThreadCalls = ThreadLocal.withInitial(() -> 0);

  private int activeCalls;
  private boolean closing;
  private boolean closed;

  public CallbackLifecycle() {}

  public synchronized Optional<Lease> enter() {
    if (closing || closed) {
      return Optional.empty();
    }
    activeCalls++;
    currentThreadCalls.set(currentThreadCalls.get() + 1);
    return Optional.of(new Lease());
  }

  public boolean isCurrentThreadInCallback() {
    return currentThreadCalls.get() > 0;
  }

  public void close(String callbackName, Runnable action) {
    Objects.requireNonNull(callbackName, "callbackName");
    Objects.requireNonNull(action, "action");
    var interrupted = false;
    var shouldClose = false;
    synchronized (this) {
      if (isCurrentThreadInCallback()) {
        throw Status.callbackReentry(callbackName);
      }
      while (closing && !closed) {
        try {
          wait();
        } catch (InterruptedException error) {
          interrupted = true;
        }
      }
      if (closed) {
        if (interrupted) {
          Thread.currentThread().interrupt();
        }
        return;
      }
      closing = true;
      while (activeCalls > 0) {
        try {
          wait();
        } catch (InterruptedException error) {
          interrupted = true;
        }
      }
      shouldClose = true;
    }
    try {
      if (shouldClose) {
        runCloseAction(action);
      }
    } finally {
      synchronized (this) {
        if (shouldClose) {
          closed = true;
          notifyAll();
        }
      }
      if (interrupted) {
        Thread.currentThread().interrupt();
      }
    }
  }

  public final class Lease implements AutoCloseable {
    private boolean closed;

    @Override
    public void close() {
      synchronized (CallbackLifecycle.this) {
        if (closed) {
          return;
        }
        closed = true;
        activeCalls--;
        var threadCalls = currentThreadCalls.get() - 1;
        if (threadCalls == 0) {
          currentThreadCalls.remove();
        } else {
          currentThreadCalls.set(threadCalls);
        }
        if (activeCalls == 0) {
          CallbackLifecycle.this.notifyAll();
        }
      }
    }
  }

  private static void runCloseAction(Runnable action) {
    for (var attempt = 0; ; attempt++) {
      try {
        action.run();
        return;
      } catch (IllegalStateException error) {
        if (!isAcquiredSession(error)) {
          throw error;
        }
        if (attempt >= MAX_DEFERRED_CLOSE_ATTEMPTS) {
          LOGGER.log(
              System.Logger.Level.WARNING,
              "Timed out waiting for FFM callback arena release",
              error);
          throw error;
        }
        sleepBeforeRetry();
      }
    }
  }

  private static void sleepBeforeRetry() {
    try {
      TimeUnit.MILLISECONDS.sleep(1);
    } catch (InterruptedException error) {
      Thread.currentThread().interrupt();
      throw new IllegalStateException("interrupted while waiting to close callback arena", error);
    }
  }

  private static boolean isAcquiredSession(IllegalStateException error) {
    var message = error.getMessage();
    return message != null
        && (message.contains("Session is acquired") || message.contains("Scope is acquired"));
  }
}
