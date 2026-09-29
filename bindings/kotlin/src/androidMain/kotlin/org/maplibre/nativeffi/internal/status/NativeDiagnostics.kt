package org.maplibre.nativeffi.internal.status

import org.maplibre.nativeffi.internal.javacpp.JavaCppSupport
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC

/**
 * Supplies the `mln_diagnostic` that each status-returning C call writes its message into.
 *
 * Each thread reuses one buffer. Native writes the message when a call exits and [check] reads it
 * right after on the same thread, so calls nested in callbacks during that call cannot clobber it.
 */
internal object NativeDiagnostics {
  private val buffers =
    object : ThreadLocal<MaplibreNativeC.mln_diagnostic>() {
      // The extra reference keeps a PointerScope open at first use from freeing the buffer. The
      // garbage collector still frees it once its thread ends.
      override fun initialValue(): MaplibreNativeC.mln_diagnostic =
        MaplibreNativeC.mln_diagnostic().retainReference()
    }

  /** Calls native with this thread's diagnostic and throws the mapped exception on failure. */
  inline fun check(call: (MaplibreNativeC.mln_diagnostic) -> Int) {
    val diagnostic = buffer()
    Status.check(call(diagnostic)) { message(diagnostic) }
  }

  /** Returns this thread's diagnostic, sized for the next call. */
  fun buffer(): MaplibreNativeC.mln_diagnostic = buffers.get().also { it.size(it.sizeof()) }

  /** Copies the message that the last call through [diagnostic] wrote. */
  fun message(diagnostic: MaplibreNativeC.mln_diagnostic): String =
    JavaCppSupport.cString(diagnostic.message())
}
