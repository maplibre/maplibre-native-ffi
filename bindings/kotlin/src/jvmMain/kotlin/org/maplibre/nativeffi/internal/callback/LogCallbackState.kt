package org.maplibre.nativeffi.internal.callback

import java.lang.foreign.Arena
import java.lang.foreign.Linker
import java.lang.foreign.MemorySegment
import java.lang.foreign.ValueLayout
import java.lang.invoke.MethodHandles
import java.lang.invoke.MethodType
import org.maplibre.nativeffi.internal.c.mln_log_callback
import org.maplibre.nativeffi.internal.c.mln_log_callback_release
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.log.LogEvent
import org.maplibre.nativeffi.log.LogRecord
import org.maplibre.nativeffi.log.LogSeverity

/** Owns process-global JVM FFM logging callback state. */
internal class LogCallbackState(private val callback: LogCallback) : AutoCloseable {
  private val arena = Arena.ofAuto()
  private val gate = CallbackGate("log callbacks") {}
  private val callbackStub: MemorySegment
  private val releaseStub: MemorySegment

  init {
    val lookup = MethodHandles.lookup()
    val callbackMethod =
      lookup
        .findVirtual(
          LogCallbackState::class.java,
          "invoke",
          MethodType.methodType(
            Int::class.javaPrimitiveType,
            MemorySegment::class.java,
            Int::class.javaPrimitiveType,
            Int::class.javaPrimitiveType,
            Long::class.javaPrimitiveType,
            MemorySegment::class.java,
          ),
        )
        .bindTo(this)
    callbackStub =
      Linker.nativeLinker().upcallStub(callbackMethod, mln_log_callback.descriptor(), arena)
    val releaseMethod =
      lookup
        .findVirtual(
          LogCallbackState::class.java,
          "release",
          MethodType.methodType(Void.TYPE, MemorySegment::class.java),
        )
        .bindTo(this)
    releaseStub =
      Linker.nativeLinker().upcallStub(releaseMethod, mln_log_callback_release.descriptor(), arena)
  }

  @Suppress("UNUSED_PARAMETER")
  fun invoke(
    userData: MemorySegment,
    rawSeverity: Int,
    rawEvent: Int,
    code: Long,
    message: MemorySegment,
  ): Int {
    val lease = gate.enter() ?: return 0
    return try {
      val record =
        LogRecord(
          LogSeverity.fromNative(rawSeverity),
          LogEvent.fromNative(rawEvent),
          code,
          copyCString(message),
        )
      if (callback.log(record)) 1 else 0
    } catch (_: Throwable) {
      0
    } finally {
      lease.close()
    }
  }

  @Suppress("UNUSED_PARAMETER")
  fun release(userData: MemorySegment) {
    HandleLeakCleaner.releaseNativeCallbackRoot(this)
    close()
  }

  override fun close() = gate.close()

  fun isClosedForTesting(): Boolean = gate.isClosedForTesting()

  private fun copyCString(address: MemorySegment): String {
    if (address == MemorySegment.NULL) return ""
    var length = 0L
    while (address.reinterpret(length + 1).get(ValueLayout.JAVA_BYTE, length) != 0.toByte()) {
      length++
    }
    return String(address.reinterpret(length).toArray(ValueLayout.JAVA_BYTE), Charsets.UTF_8)
  }

  internal companion object {
    fun set(callback: LogCallback) {
      NativeAccess.ensureLoaded()
      val replacement = LogCallbackState(callback)
      HandleLeakCleaner.retainNativeCallbackRoot(replacement)
      try {
        Status.check(NativeAccess.setLogCallback(replacement.callbackStub, replacement.releaseStub))
      } catch (error: Throwable) {
        HandleLeakCleaner.releaseNativeCallbackRoot(replacement)
        replacement.close()
        throw error
      }
    }

    fun clear() {
      NativeAccess.ensureLoaded()
      Status.check(NativeAccess.clearLogCallback())
    }

    fun createForTesting(callback: LogCallback): LogCallbackState = LogCallbackState(callback)
  }
}
