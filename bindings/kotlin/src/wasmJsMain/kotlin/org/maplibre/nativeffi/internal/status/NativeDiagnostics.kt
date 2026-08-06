package org.maplibre.nativeffi.internal.status

import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.HeapPointer
import org.maplibre.nativeffi.internal.wasm.generated.mln_thread_last_error_message

internal actual object NativeDiagnostics {
  actual fun currentDiagnostic(): String =
    Heap.loadUtf8(HeapPointer(mln_thread_last_error_message()))
}
