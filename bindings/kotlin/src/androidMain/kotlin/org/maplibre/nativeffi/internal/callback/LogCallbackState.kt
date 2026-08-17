package org.maplibre.nativeffi.internal.callback

import org.bytedeco.javacpp.BytePointer
import org.bytedeco.javacpp.Pointer
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.internal.javacpp.JavaCppSupport
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.log.LogEvent
import org.maplibre.nativeffi.log.LogRecord
import org.maplibre.nativeffi.log.LogSeverity

/** Owns process-global Android JNI logging callback state. */
internal class LogCallbackState(private val callback: LogCallback) : AutoCloseable {
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
  private val nativeRelease =
    object : MaplibreNativeC.mln_log_callback_release() {
      override fun call(userData: Pointer?) {
        HandleLeakCleaner.releaseNativeCallbackRoot(this@LogCallbackState)
        close()
      }
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

  override fun close() = gate.close()

  fun isClosedForTesting(): Boolean = gate.isClosedForTesting()

  internal companion object {
    fun set(callback: LogCallback) {
      NativeAccess.ensureLoaded()
      val replacement = LogCallbackState(callback)
      HandleLeakCleaner.retainNativeCallbackRoot(replacement)
      try {
        Status.check(
          MaplibreNativeC.mln_log_set_callback(
            replacement.nativeCallback,
            null,
            replacement.nativeRelease,
          )
        )
      } catch (error: Throwable) {
        HandleLeakCleaner.releaseNativeCallbackRoot(replacement)
        replacement.close()
        throw error
      }
    }

    fun clear() {
      NativeAccess.ensureLoaded()
      Status.check(MaplibreNativeC.mln_log_clear_callback())
    }

    fun createForTesting(callback: LogCallback): LogCallbackState = LogCallbackState(callback)
  }
}
