package org.maplibre.nativeffi.internal.callback

import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.HeapPointer
import org.maplibre.nativeffi.internal.wasm.InjectedFaults
import org.maplibre.nativeffi.internal.wasm.generated.MlnAdapterLogRecord
import org.maplibre.nativeffi.internal.wasm.generated.mln_adapter_log_record_destroy
import org.maplibre.nativeffi.internal.wasm.generated.mln_kotlin_log_clear
import org.maplibre.nativeffi.internal.wasm.generated.mln_kotlin_log_install
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.log.LogEvent
import org.maplibre.nativeffi.log.LogRecord
import org.maplibre.nativeffi.log.LogSeverity

/** Owns the process-global log callback registration. */
internal object LogCallbackState {
  private const val SUBJECT = "log callbacks"
  private const val INSTALL = "mln_kotlin_log_install"
  private const val CLEAR = "mln_kotlin_log_clear"

  /** The registration that records produced now belong to. */
  private var current: Registration? = null

  /**
   * The registrations a clear retired, oldest first.
   *
   * A cleared registration keeps receiving until its marker comes out of the ring, because every
   * record ahead of that marker is one it was installed for.
   */
  private val retiring = ArrayDeque<Registration>()

  fun set(callback: LogCallback, consume: Boolean) {
    current?.checkCanClose()
    val replacement = Registration(callback)
    InjectedFaults.beginCall(INSTALL)
    // Installed over the previous registration rather than cleared first: the shim identifies its
    // registration by one state address, so a clear would leave native logging to nobody until the
    // install landed, and a refused install would leave it that way for good.
    Status.check(mln_kotlin_log_install(if (consume) 1 else 0))
    val previous = current
    current = replacement
    // Native saw no retirement, so no marker is coming and the replaced registration stops here.
    previous?.close()
  }

  fun clear() {
    current?.checkCanClose()
    InjectedFaults.beginCall(CLEAR)
    Status.check(mln_kotlin_log_clear())
    current?.let { retiring.addLast(it) }
    current = null
  }

  /** Delivers one `mln_adapter_log_record` and releases it. */
  fun deliver(record: HeapPointer) {
    try {
      val target = retiring.firstOrNull() ?: current ?: return
      target.deliver(
        LogRecord(
          LogSeverity.fromNative(MlnAdapterLogRecord.severity(record)),
          LogEvent.fromNative(MlnAdapterLogRecord.event(record)),
          MlnAdapterLogRecord.code(record),
          Heap.loadUtf8(MlnAdapterLogRecord.message(record)),
        )
      )
    } finally {
      mln_adapter_log_record_destroy(record.address)
    }
  }

  /**
   * Retires the oldest cleared registration, which every record ahead of the marker belonged to.
   */
  fun retired() {
    retiring.removeFirstOrNull()?.close()
  }

  /** One host callback's registration, which outlives its native registration by the clear. */
  private class Registration(private val callback: LogCallback) {
    private val gate = CallbackGate(SUBJECT)

    fun deliver(record: LogRecord) {
      val lease = gate.enter() ?: return
      try {
        // Contained: a failing callback must not stop the drain, and no frame above it is native.
        runCatching { callback.log(record) }
      } finally {
        lease.close()
      }
    }

    fun checkCanClose() = gate.checkCanClose()

    fun close() = gate.close()
  }
}
