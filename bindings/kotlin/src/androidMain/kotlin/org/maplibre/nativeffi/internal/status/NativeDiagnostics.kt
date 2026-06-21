package org.maplibre.nativeffi.internal.status

import org.maplibre.nativeffi.internal.javacpp.JavaCppSupport
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC

internal actual object NativeDiagnostics {
  actual fun currentDiagnostic(): String =
    try {
      JavaCppSupport.takeThreadDiagnostic()
        ?: JavaCppSupport.cString(MaplibreNativeC.mln_thread_last_error_message())
    } catch (_: UnsatisfiedLinkError) {
      ""
    }
}
