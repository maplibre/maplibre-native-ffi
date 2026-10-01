package org.maplibre.nativeffi.internal.status

import org.maplibre.nativeffi.internal.lifecycle.UnreachableActions
import org.maplibre.nativeffi.internal.memory.NativeMemory

private class DiagnosticBuffer {
  val address: Long = NativeMemory.allocate(NativeDiagnostics.SIZE.toLong())

  init {
    // Frees the buffer once its thread ends and drops the thread-local reference.
    val address = address
    UnreachableActions.register(this, Runnable { NativeMemory.free(address) })
  }
}

// ThreadLocal.withInitial needs API 26, above the binding's floor.
private val buffers =
  object : ThreadLocal<DiagnosticBuffer>() {
    override fun initialValue(): DiagnosticBuffer = DiagnosticBuffer()
  }

internal actual fun threadDiagnostic(): Long = buffers.get()!!.address
