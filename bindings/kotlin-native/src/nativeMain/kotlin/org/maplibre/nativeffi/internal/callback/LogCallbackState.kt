package org.maplibre.nativeffi.internal.callback

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.StableRef
import kotlinx.cinterop.asStableRef
import kotlinx.cinterop.staticCFunction
import org.maplibre.nativeffi.internal.c.mln_log_clear_callback
import org.maplibre.nativeffi.internal.c.mln_log_set_callback
import org.maplibre.nativeffi.internal.memory.MemoryUtil
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.log.LogEvent
import org.maplibre.nativeffi.log.LogRecord
import org.maplibre.nativeffi.log.LogSeverity

/** Owns process-global logging callback state. */
@OptIn(ExperimentalForeignApi::class, ExperimentalAtomicApi::class)
internal class LogCallbackState private constructor(private val callback: LogCallback) :
  AutoCloseable {
  private val selfRef = StableRef.create(this)
  private val closed = AtomicInt(0)

  fun userData(): COpaquePointer = selfRef.asCPointer()

  fun invoke(severity: UInt, event: UInt, code: Long, message: CPointer<ByteVar>?): UInt {
    if (closed.load() != 0) return 0U
    return try {
      val record =
        LogRecord(
          LogSeverity.fromNative(severity),
          severity.toInt(),
          LogEvent.fromNative(event),
          event.toInt(),
          code,
          MemoryUtil.copyCString(message),
        )
      if (callback.log(record)) 1U else 0U
    } catch (_: Throwable) {
      0U
    }
  }

  override fun close() {
    // Native logging can dispatch from worker threads. The C API stops future callbacks after
    // replacement or clear, but it does not guarantee that an already-entered upcall has finished
    // reading user_data. Keep the StableRef allocated after retirement and gate dispatch instead.
    closed.store(1)
  }

  internal companion object {
    private val lock = AtomicInt(0)
    private var current: LogCallbackState? = null
    private val retired = mutableListOf<LogCallbackState>()

    fun set(callback: LogCallback) {
      val replacement = LogCallbackState(callback)
      var previous: LogCallbackState? = null
      try {
        withLock {
          Status.check(mln_log_set_callback(staticCFunction(::logCallback), replacement.userData()))
          previous = current
          current = replacement
        }
      } catch (error: Throwable) {
        replacement.close()
        throw error
      }
      retire(previous)
    }

    fun clear() {
      var previous: LogCallbackState? = null
      withLock {
        Status.check(mln_log_clear_callback())
        previous = current
        current = null
      }
      retire(previous)
    }

    fun currentForTesting(): LogCallbackState? = withLock { current }

    private fun retire(state: LogCallbackState?) {
      if (state == null) return
      state.close()
      withLock { retired += state }
    }

    private inline fun <T> withLock(block: () -> T): T {
      while (!lock.compareAndSet(0, 1)) {
        // Process-global logging callback replacement is rare; spin briefly.
      }
      try {
        return block()
      } finally {
        lock.store(0)
      }
    }
  }
}

@OptIn(ExperimentalForeignApi::class)
private fun logCallback(
  userData: COpaquePointer?,
  severity: UInt,
  event: UInt,
  code: Long,
  message: CPointer<ByteVar>?,
): UInt =
  userData?.asStableRef<LogCallbackState>()?.get()?.invoke(severity, event, code, message) ?: 0U
