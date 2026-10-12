package org.maplibre.nativeffi.internal.status

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.concurrent.ThreadLocal
import kotlin.native.ref.createCleaner
import org.maplibre.nativeffi.internal.memory.NativeMemory

@OptIn(ExperimentalNativeApi::class)
private class DiagnosticBuffer {
  val address: Long = NativeMemory.allocate(NativeDiagnostics.SIZE.toLong())

  // Frees the buffer once its thread ends and drops the thread-local reference.
  @Suppress("unused") private val cleaner = createCleaner(address) { NativeMemory.free(it) }
}

@ThreadLocal private val threadBuffer = DiagnosticBuffer()

internal actual fun threadDiagnostic(): Long = threadBuffer.address
