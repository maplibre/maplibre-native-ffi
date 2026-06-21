package org.maplibre.nativeffi.internal.callback

import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.staticCFunction
import org.maplibre.nativeffi.internal.c.mln_log_clear_callback
import org.maplibre.nativeffi.internal.c.mln_log_set_callback
import org.maplibre.nativeffi.internal.memory.MemoryUtil
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.log.LogEvent
import org.maplibre.nativeffi.log.LogRecord
import org.maplibre.nativeffi.log.LogSeverity

/** Owns process-global logging callback state. */
@OptIn(ExperimentalForeignApi::class)
internal class LogCallbackState private constructor(private val callback: LogCallback) :
  AutoCloseable {
  private val gate = CallbackGate("log callbacks")

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

  internal fun checkCanClose() = gate.checkCanClose()

  internal fun isClosedForTesting(): Boolean = gate.isClosedForTesting()

  internal companion object {
    private val registry = LogCallbackRegistry<LogCallbackState>()

    fun set(callback: LogCallback) {
      registry.current()?.checkCanClose()
      registry.set(LogCallbackState(callback)) {
        mln_log_set_callback(staticCFunction(::logCallback), null)
      }
    }

    fun setForTesting(
      callback: LogCallback,
      install: () -> Int,
      captureReplacement: (LogCallbackState) -> Unit,
    ) {
      registry.current()?.checkCanClose()
      registry.set(LogCallbackState(callback).also(captureReplacement), install)
    }

    fun clear() {
      registry.current()?.checkCanClose()
      registry.clear(::mln_log_clear_callback)
    }

    fun currentForTesting(): LogCallbackState? = registry.current()

    fun invokeCurrent(severity: UInt, event: UInt, code: Long, message: CPointer<ByteVar>?): UInt =
      registry.current()?.invoke(severity, event, code, message) ?: 0U
  }
}

@Suppress("UNUSED_PARAMETER")
@OptIn(ExperimentalForeignApi::class)
private fun logCallback(
  userData: COpaquePointer?,
  severity: UInt,
  event: UInt,
  code: Long,
  message: CPointer<ByteVar>?,
): UInt = LogCallbackState.invokeCurrent(severity, event, code, message)
