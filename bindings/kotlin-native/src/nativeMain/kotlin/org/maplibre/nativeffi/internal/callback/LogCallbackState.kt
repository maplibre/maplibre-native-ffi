package org.maplibre.nativeffi.internal.callback

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
@OptIn(ExperimentalForeignApi::class)
internal class LogCallbackState private constructor(private val callback: LogCallback) :
  AutoCloseable {
  private val selfRef = StableRef.create(this)
  private var closed = false

  fun userData(): COpaquePointer = selfRef.asCPointer()

  fun invoke(severity: UInt, event: UInt, code: Long, message: CPointer<ByteVar>?): UInt {
    if (closed) return 0U
    return try {
      val record =
        LogRecord(
          LogSeverity.fromNative(severity),
          severity,
          LogEvent.fromNative(event),
          event,
          code,
          MemoryUtil.copyCString(message),
        )
      if (callback.log(record)) 1U else 0U
    } catch (_: Throwable) {
      0U
    }
  }

  override fun close() {
    if (closed) return
    closed = true
    selfRef.dispose()
  }

  internal companion object {
    private var current: LogCallbackState? = null

    fun set(callback: LogCallback) {
      val replacement = LogCallbackState(callback)
      val previous: LogCallbackState?
      try {
        Status.check(mln_log_set_callback(staticCFunction(::logCallback), replacement.userData()))
        previous = current
        current = replacement
      } catch (error: Throwable) {
        replacement.close()
        throw error
      }
      previous?.close()
    }

    fun clear() {
      Status.check(mln_log_clear_callback())
      val previous = current
      current = null
      previous?.close()
    }

    fun currentForTesting(): LogCallbackState? = current
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
