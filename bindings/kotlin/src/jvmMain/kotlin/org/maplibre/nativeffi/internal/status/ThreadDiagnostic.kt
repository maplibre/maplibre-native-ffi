package org.maplibre.nativeffi.internal.status

import java.lang.foreign.Arena
import java.lang.foreign.MemorySegment

// An automatic arena frees each thread's buffer once the thread and its thread local are gone.
private val buffers: ThreadLocal<MemorySegment> = ThreadLocal.withInitial {
  Arena.ofAuto().allocate(NativeDiagnostics.SIZE.toLong(), 4)
}

internal actual fun threadDiagnostic(): Long = buffers.get().address()
