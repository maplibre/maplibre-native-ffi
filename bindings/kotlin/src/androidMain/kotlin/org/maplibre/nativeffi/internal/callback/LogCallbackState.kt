package org.maplibre.nativeffi.internal.callback

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.bytedeco.javacpp.BytePointer
import org.bytedeco.javacpp.Pointer
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.internal.javacpp.JavaCppSupport
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.log.LogEvent
import org.maplibre.nativeffi.log.LogRecord
import org.maplibre.nativeffi.log.LogSeverity

/** Owns process-global Android JNI logging callback state. */
@OptIn(ExperimentalAtomicApi::class)
internal class LogCallbackState private constructor(private val callback: LogCallback) :
  AutoCloseable {
  private val gate = CallbackGate("log callbacks") { nativeCallback.close() }
  private val nativeCallback =
    object : MaplibreNativeC.mln_log_callback() {
      override fun call(
        userData: Pointer?,
        severity: Int,
        event: Int,
        code: Long,
        message: BytePointer?,
      ): Int = invoke(severity, event, code, message)
    }

  fun invoke(rawSeverity: Int, rawEvent: Int, code: Long, message: BytePointer?): Int {
    val lease = gate.enter() ?: return 0
    return try {
      val record =
        LogRecord(
          LogSeverity.fromNative(rawSeverity),
          LogEvent.fromNative(rawEvent),
          code,
          JavaCppSupport.cString(message),
        )
      if (callback.log(record)) 1 else 0
    } catch (_: Throwable) {
      0
    } finally {
      lease.close()
    }
  }

  fun checkCanClose() = gate.checkCanClose()

  override fun close() = gate.close()

  internal companion object {
    private val updateLock = AtomicInt(0)
    private val current = AtomicReference<LogCallbackState?>(null)

    fun set(callback: LogCallback) {
      NativeAccess.ensureLoaded()
      val replacement = LogCallbackState(callback)
      var previous: LogCallbackState? = null
      try {
        withUpdateLock {
          current.load()?.checkCanClose()
          Status.check(MaplibreNativeC.mln_log_set_callback(replacement.nativeCallback, null))
          previous = current.exchange(replacement)
        }
      } catch (error: Throwable) {
        closeAndSuppress(error, replacement)
        throw error
      }
      closeQuietly(previous)
    }

    fun clear() {
      NativeAccess.ensureLoaded()
      var previous: LogCallbackState? = null
      withUpdateLock {
        current.load()?.checkCanClose()
        Status.check(MaplibreNativeC.mln_log_clear_callback())
        previous = current.exchange(null)
      }
      closeQuietly(previous)
    }

    fun currentForTesting(): LogCallbackState? = current.load()

    private fun closeQuietly(state: LogCallbackState?) {
      try {
        state?.close()
      } catch (_: RuntimeException) {}
    }

    private fun closeAndSuppress(error: Throwable, state: LogCallbackState?) {
      try {
        state?.close()
      } catch (cleanup: Throwable) {
        error.addSuppressed(cleanup)
      }
    }

    private inline fun <R> withUpdateLock(block: () -> R): R {
      while (!updateLock.compareAndSet(0, 1)) {
        // Spin briefly; log callback registration is process-global and infrequent.
      }
      try {
        return block()
      } finally {
        updateLock.store(0)
      }
    }
  }
}
