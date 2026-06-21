package org.maplibre.nativeffi.internal.callback

import java.lang.foreign.Arena
import java.lang.foreign.Linker
import java.lang.foreign.MemorySegment
import java.lang.foreign.ValueLayout
import java.lang.invoke.MethodHandles
import java.lang.invoke.MethodType
import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.internal.c.mln_log_callback
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.log.LogEvent
import org.maplibre.nativeffi.log.LogRecord
import org.maplibre.nativeffi.log.LogSeverity

/** Owns process-global JVM FFM logging callback state. */
@OptIn(ExperimentalAtomicApi::class)
internal class LogCallbackState private constructor(private val callback: LogCallback) :
  AutoCloseable {
  private val arena = Arena.ofShared()
  private val gate = CallbackGate("log callbacks") { arena.close() }
  private val stub: MemorySegment

  init {
    val method =
      MethodHandles.lookup()
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
    stub = Linker.nativeLinker().upcallStub(method, callbackDescriptor, arena)
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

  fun checkCanClose() = gate.checkCanClose()

  override fun close() = gate.close()

  private fun copyCString(address: MemorySegment): String {
    if (address == MemorySegment.NULL) {
      return ""
    }
    var length = 0L
    while (address.reinterpret(length + 1).get(ValueLayout.JAVA_BYTE, length) != 0.toByte()) {
      length++
    }
    return String(address.reinterpret(length).toArray(ValueLayout.JAVA_BYTE), Charsets.UTF_8)
  }

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
          Status.check(NativeAccess.setLogCallback(replacement.stub))
          previous = current.exchange(replacement)
        }
      } catch (error: Throwable) {
        replacement.close()
        throw error
      }
      closeQuietly(previous)
    }

    fun clear() {
      NativeAccess.ensureLoaded()
      var previous: LogCallbackState? = null
      withUpdateLock {
        current.load()?.checkCanClose()
        Status.check(NativeAccess.clearLogCallback())
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

    private inline fun <R> withUpdateLock(block: () -> R): R {
      while (!updateLock.compareAndSet(0, 1)) {}
      try {
        return block()
      } finally {
        updateLock.store(0)
      }
    }

    private val callbackDescriptor = mln_log_callback.descriptor()
  }
}
