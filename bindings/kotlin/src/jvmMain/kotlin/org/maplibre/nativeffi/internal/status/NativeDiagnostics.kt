package org.maplibre.nativeffi.internal.status

import java.lang.foreign.Arena
import java.lang.foreign.MemorySegment
import java.nio.charset.StandardCharsets
import org.maplibre.nativeffi.internal.c.mln_diagnostic

/**
 * Supplies the `mln_diagnostic` that each status-returning C call writes its message into.
 *
 * Each thread reuses one buffer. Native writes the message when a call exits and [check] reads it
 * right after on the same thread, so calls nested in callbacks during that call cannot clobber it.
 */
internal object NativeDiagnostics {
  private val SIZE = mln_diagnostic.sizeof().toInt()
  private val buffers = ThreadLocal.withInitial { mln_diagnostic.allocate(Arena.ofAuto()) }

  /** Calls native with this thread's diagnostic and throws the mapped exception on failure. */
  inline fun check(call: (MemorySegment) -> Int) {
    val diagnostic = buffer()
    Status.check(call(diagnostic)) { message(diagnostic) }
  }

  /** Returns this thread's diagnostic, sized for the next call. */
  fun buffer(): MemorySegment = buffers.get().also { mln_diagnostic.size(it, SIZE) }

  /** Copies the message that the last call through [diagnostic] wrote. */
  fun message(diagnostic: MemorySegment): String =
    mln_diagnostic.message(diagnostic).getString(0, StandardCharsets.UTF_8)
}
