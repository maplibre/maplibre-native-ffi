package org.maplibre.nativeffi.internal.callback

import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.atomic.AtomicLong
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
  private val token = TOKENS.getAndIncrement()
  private val gate = CallbackGate("log callbacks") { STATES.remove(token) }

  init {
    STATES[token] = this
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
            NATIVE_CALLBACK,
            JavaCppSupport.addressPointer(replacement.token),
            NATIVE_RELEASE,
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

    private val TOKENS = AtomicLong(1)
    private val STATES = ConcurrentHashMap<Long, LogCallbackState>()

    /**
     * One process-wide thunk. JavaCPP's FunctionPointer pool is ten slots per generated class, so
     * per-registration thunks would fail once eleven log callbacks were live at once.
     */
    private val NATIVE_CALLBACK =
      object : MaplibreNativeC.mln_log_callback() {
        override fun call(
          userData: Pointer?,
          severity: Int,
          event: Int,
          code: Long,
          message: BytePointer?,
        ): Int = STATES[userData?.address() ?: 0L]?.invoke(severity, event, code, message) ?: 0
      }

    /** One process-wide release thunk; per-state thunks would exhaust the same pool. */
    private val NATIVE_RELEASE =
      object : MaplibreNativeC.mln_log_callback_release() {
        override fun call(userData: Pointer?) {
          val state = STATES[userData?.address() ?: 0L] ?: return
          HandleLeakCleaner.releaseNativeCallbackRoot(state)
          state.close()
        }
      }
  }
}
