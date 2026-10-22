package org.maplibre.nativeffi.internal.status

import org.maplibre.nativeffi.internal.memory.NativeMemory

// Kotlin runs on one thread in the browser, so one buffer serves every call. The module never
// unloads, so the buffer stays allocated.
private var buffer = 0L

internal actual fun threadDiagnostic(): Long {
  if (buffer == 0L) buffer = NativeMemory.allocate(NativeDiagnostics.SIZE.toLong())
  return buffer
}
