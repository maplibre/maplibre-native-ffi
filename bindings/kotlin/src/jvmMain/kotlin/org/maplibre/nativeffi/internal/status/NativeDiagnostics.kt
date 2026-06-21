package org.maplibre.nativeffi.internal.status

import java.lang.foreign.FunctionDescriptor
import java.lang.foreign.Linker
import java.lang.foreign.MemorySegment
import java.lang.foreign.SymbolLookup
import java.nio.charset.StandardCharsets
import java.util.NoSuchElementException

internal actual object NativeDiagnostics {
  actual fun currentDiagnostic(): String =
    try {
      currentNativeDiagnostic()
    } catch (_: IllegalCallerException) {
      ""
    } catch (_: NoSuchElementException) {
      ""
    } catch (_: UnsatisfiedLinkError) {
      ""
    }

  private fun currentNativeDiagnostic(): String {
    val symbol =
      SymbolLookup.loaderLookup().find("mln_thread_last_error_message").orElseThrow {
        NoSuchElementException("mln_thread_last_error_message")
      }
    val address =
      Linker.nativeLinker()
        .downcallHandle(symbol, FunctionDescriptor.of(java.lang.foreign.ValueLayout.ADDRESS))
        .invokeWithArguments() as MemorySegment
    return if (address == MemorySegment.NULL) {
      ""
    } else {
      address.reinterpret(Long.MAX_VALUE).getString(0, StandardCharsets.UTF_8)
    }
  }
}
