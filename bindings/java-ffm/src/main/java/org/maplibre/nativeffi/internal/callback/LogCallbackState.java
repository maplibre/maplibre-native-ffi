package org.maplibre.nativeffi.internal.callback;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.util.Objects;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_log_callback;
import org.maplibre.nativeffi.internal.convert.NativeValues;
import org.maplibre.nativeffi.internal.loader.NativeAccess;
import org.maplibre.nativeffi.internal.memory.MemoryUtil;
import org.maplibre.nativeffi.internal.status.Status;
import org.maplibre.nativeffi.log.LogCallback;
import org.maplibre.nativeffi.log.LogRecord;

/** Owns process-global logging callback state. */
public final class LogCallbackState implements AutoCloseable {
  private static final Object LOCK = new Object();
  private static LogCallbackState current;

  private final Arena arena;
  private final CallbackLifecycle lifecycle = new CallbackLifecycle();
  private final LogCallback callback;
  private final MemorySegment stub;

  private LogCallbackState(LogCallback callback) {
    this.arena = Arena.ofShared();
    this.callback = callback;
    this.stub = mln_log_callback.allocate(this::invoke, arena);
  }

  public static void set(LogCallback callback) {
    NativeAccess.ensureLoaded();
    var replacement = new LogCallbackState(Objects.requireNonNull(callback, "callback"));
    LogCallbackState previous;
    try {
      synchronized (LOCK) {
        if (current != null && current.isCurrentThreadInCallback()) {
          throw Status.callbackReentry("Log callback");
        }
        Status.check(MapLibreNativeC.mln_log_set_callback(replacement.stub, MemorySegment.NULL));
        previous = current;
        current = replacement;
      }
    } catch (RuntimeException | Error error) {
      closeQuietly(replacement);
      throw error;
    }
    closeQuietly(previous);
  }

  public static void clear() {
    NativeAccess.ensureLoaded();
    LogCallbackState previous;
    synchronized (LOCK) {
      if (current != null && current.isCurrentThreadInCallback()) {
        throw Status.callbackReentry("Log callback");
      }
      Status.check(MapLibreNativeC.mln_log_clear_callback());
      previous = current;
      current = null;
    }
    closeQuietly(previous);
  }

  static LogCallbackState currentForTesting() {
    synchronized (LOCK) {
      return current;
    }
  }

  MemorySegment stubForTesting() {
    return stub;
  }

  boolean isCurrentThreadInCallback() {
    return lifecycle.isCurrentThreadInCallback();
  }

  private int invoke(
      MemorySegment userData, int severity, int event, long code, MemorySegment message) {
    var lease = lifecycle.enter();
    if (lease.isEmpty()) {
      return 0;
    }
    try (var ignored = lease.get()) {
      var record =
          new LogRecord(
              NativeValues.logSeverity(severity),
              severity,
              NativeValues.logEvent(event),
              event,
              code,
              MemoryUtil.copyCString(message));
      return callback.log(record) ? 1 : 0;
    } catch (Throwable ignored) {
      return 0;
    }
  }

  @Override
  public void close() {
    lifecycle.close("Log callback", arena::close);
  }

  private static void closeQuietly(LogCallbackState state) {
    if (state == null) {
      return;
    }
    try {
      state.close();
    } catch (IllegalStateException ignored) {
      // Another native thread may be finishing an upcall. The process will reclaim this arena.
    }
  }
}
