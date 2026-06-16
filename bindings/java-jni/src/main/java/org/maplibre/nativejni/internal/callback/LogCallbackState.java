package org.maplibre.nativejni.internal.callback;

import java.util.Objects;
import java.util.concurrent.atomic.AtomicReference;
import org.bytedeco.javacpp.BytePointer;
import org.bytedeco.javacpp.Pointer;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.internal.loader.NativeLibrary;
import org.maplibre.nativejni.internal.status.Status;
import org.maplibre.nativejni.log.LogCallback;
import org.maplibre.nativejni.log.LogEvent;
import org.maplibre.nativejni.log.LogRecord;
import org.maplibre.nativejni.log.LogSeverity;

/** Owns process-global logging callback state. */
public final class LogCallbackState {
  private static CallbackRegistration currentCallback;
  private static CallbackRegistration failedInstallForTesting;
  private static final AtomicReference<RuntimeException> INSTALL_FAILURE = new AtomicReference<>();

  private LogCallbackState() {}

  public static synchronized void set(LogCallback callback) {
    NativeLibrary.ensureLoaded();
    if (currentCallback != null && currentCallback.isCurrentThreadInCallback()) {
      throw Status.callbackReentry("Log callback");
    }
    var nativeCallback = new CallbackRegistration(callback);
    try {
      throwInstallFailureForTesting();
      Status.check(MaplibreNativeC.mln_log_set_callback(nativeCallback.nativeCallback(), null));
    } catch (RuntimeException | Error error) {
      closeQuietly(nativeCallback);
      failedInstallForTesting = nativeCallback;
      throw error;
    }
    closeQuietly(currentCallback);
    currentCallback = nativeCallback;
  }

  public static synchronized void clear() {
    NativeLibrary.ensureLoaded();
    if (currentCallback != null && currentCallback.isCurrentThreadInCallback()) {
      throw Status.callbackReentry("Log callback");
    }
    Status.check(MaplibreNativeC.mln_log_clear_callback());
    closeQuietly(currentCallback);
    currentCallback = null;
  }

  static synchronized CallbackRegistration currentCallbackForTesting() {
    return currentCallback;
  }

  static synchronized CallbackRegistration failedInstallForTesting() {
    return failedInstallForTesting;
  }

  static void failNextInstallForTesting(RuntimeException failure) {
    if (!INSTALL_FAILURE.compareAndSet(null, Objects.requireNonNull(failure))) {
      throw new IllegalStateException("log callback install failure is already armed");
    }
  }

  private static void throwInstallFailureForTesting() {
    var failure = INSTALL_FAILURE.getAndSet(null);
    if (failure != null) {
      throw failure;
    }
  }

  private static void closeQuietly(CallbackRegistration callback) {
    if (callback == null) {
      return;
    }
    try {
      callback.close();
    } catch (Exception ignored) {
      // Closing callback stubs is best-effort during replacement or teardown.
    }
  }

  static final class CallbackRegistration implements AutoCloseable {
    private final LogCallback callback;
    private final MaplibreNativeC.mln_log_callback nativeCallback;
    private final Object lock = new Object();
    private final ThreadLocal<Integer> callbackDepth = ThreadLocal.withInitial(() -> 0);

    private int activeCallbacks;
    private boolean closing;
    private boolean closed;

    CallbackRegistration(LogCallback callback) {
      this.callback = Objects.requireNonNull(callback, "callback");
      this.nativeCallback =
          new MaplibreNativeC.mln_log_callback() {
            @Override
            public int call(
                Pointer userData, int severity, int event, long code, BytePointer message) {
              return CallbackRegistration.this.call(severity, event, code, message);
            }
          };
    }

    MaplibreNativeC.mln_log_callback nativeCallback() {
      return nativeCallback;
    }

    boolean isClosed() {
      synchronized (lock) {
        return closed;
      }
    }

    boolean isCurrentThreadInCallback() {
      return callbackDepth.get() > 0;
    }

    private int call(int severity, int event, long code, BytePointer message) {
      if (!enterCallback()) {
        return 0;
      }
      try {
        return callback.log(
                new LogRecord(
                    LogSeverity.fromNative(severity),
                    LogEvent.fromNative(event),
                    code,
                    JavaCppSupport.cString(message)))
            ? 1
            : 0;
      } catch (Throwable exception) {
        return 0;
      } finally {
        exitCallback();
      }
    }

    private boolean enterCallback() {
      synchronized (lock) {
        if (closing || closed) {
          return false;
        }
        activeCallbacks++;
        callbackDepth.set(callbackDepth.get() + 1);
        return true;
      }
    }

    private void exitCallback() {
      synchronized (lock) {
        var depth = callbackDepth.get() - 1;
        if (depth == 0) {
          callbackDepth.remove();
        } else {
          callbackDepth.set(depth);
        }
        activeCallbacks--;
        if (activeCallbacks == 0) {
          lock.notifyAll();
        }
      }
    }

    @Override
    public void close() {
      var interrupted = false;
      synchronized (lock) {
        if (isCurrentThreadInCallback()) {
          throw Status.callbackReentry("Log callback");
        }
        if (closed) {
          return;
        }
        closing = true;
        while (activeCallbacks > 0) {
          try {
            lock.wait();
          } catch (InterruptedException exception) {
            interrupted = true;
          }
        }
        closed = true;
      }
      try {
        nativeCallback.close();
      } finally {
        if (interrupted) {
          Thread.currentThread().interrupt();
        }
      }
    }
  }
}
