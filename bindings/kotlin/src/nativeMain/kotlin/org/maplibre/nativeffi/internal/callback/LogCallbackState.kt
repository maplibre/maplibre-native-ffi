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
internal class LogCallbackState(private val callback: LogCallback) : AutoCloseable {
  private val selfRef = StableRef.create(this)
  private val gate = CallbackGate("log callbacks") { selfRef.dispose() }

  fun invoke(severity: UInt, event: UInt, code: Long, message: CPointer<ByteVar>?): UInt {
    val lease = gate.enter() ?: return 0U
    return try {
      val record =
        LogRecord(
          LogSeverity.fromNative(severity),
          LogEvent.fromNative(event),
          code,
          MemoryUtil.copyCString(message),
        )
      if (callback.log(record)) 1U else 0U
    } catch (_: Throwable) {
      0U
    } finally {
      lease.close()
    }
  }

  override fun close() = gate.close()

  internal fun isClosedForTesting(): Boolean = gate.isClosedForTesting()

  internal companion object {
    fun set(callback: LogCallback) {
      val replacement = LogCallbackState(callback)
      try {
        Status.check(
          mln_log_set_callback(
            staticCFunction(::logCallback),
            replacement.selfRef.asCPointer(),
            staticCFunction(::releaseLogCallback),
          )
        )
      } catch (error: Throwable) {
        replacement.close()
        throw error
      }
    }

    fun clear() {
      Status.check(mln_log_clear_callback())
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

@OptIn(ExperimentalForeignApi::class)
private fun releaseLogCallback(userData: COpaquePointer?) {
  userData?.asStableRef<LogCallbackState>()?.get()?.close()
}
