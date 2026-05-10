package org.maplibre.nativeffi.internal;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.util.Objects;
import org.maplibre.nativeffi.LogCallback;
import org.maplibre.nativeffi.LogEvent;
import org.maplibre.nativeffi.LogRecord;
import org.maplibre.nativeffi.LogSeverity;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_log_callback;

/** Owns process-global logging callback state. */
public final class LogCallbackState implements AutoCloseable {
  private static final Object LOCK = new Object();
  private static LogCallbackState current;

  private final Arena arena;
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

  private int invoke(
      MemorySegment userData, int severity, int event, long code, MemorySegment message) {
    try {
      var record =
          new LogRecord(
              LogSeverity.fromNative(severity),
              severity,
              LogEvent.fromNative(event),
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
    arena.close();
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
