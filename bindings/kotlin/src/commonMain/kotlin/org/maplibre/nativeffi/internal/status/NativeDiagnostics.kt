package org.maplibre.nativeffi.internal.status

import org.maplibre.nativeffi.internal.c.DiagnosticLayout
import org.maplibre.nativeffi.internal.memory.readCString
import org.maplibre.nativeffi.internal.memory.writeU32

/**
 * Supplies the `mln_diagnostic` that each status-returning C call writes its message into.
 *
 * Each thread reuses one buffer. Native writes the message when a call exits and [check] reads it
 * right after on the same thread, so calls nested in callbacks during that call cannot clobber it.
 */
internal object NativeDiagnostics {
  /** `sizeof(mln_diagnostic)`, from the generated layout. */
  val SIZE: Int
    get() = DiagnosticLayout.SIZEOF

  /** Calls native with this thread's diagnostic and throws the mapped exception on failure. */
  inline fun check(call: (Long) -> Int) {
    val diagnostic = buffer()
    Status.check(call(diagnostic)) { message(diagnostic) }
  }

  /** Returns this thread's diagnostic, sized for the next call. */
  fun buffer(): Long = threadDiagnostic().also { writeU32(it, SIZE.toUInt()) }

  /** Copies the message that the last call through [diagnostic] wrote. */
  fun message(diagnostic: Long): String = readCString(diagnostic + DiagnosticLayout.MESSAGE)
}

/** This thread's [NativeDiagnostics.SIZE]-byte buffer, freed once the thread ends. */
internal expect fun threadDiagnostic(): Long
