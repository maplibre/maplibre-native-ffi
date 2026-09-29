package org.maplibre.nativeffi.internal.status

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.concurrent.ThreadLocal
import kotlin.native.ref.createCleaner
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.free
import kotlinx.cinterop.nativeHeap
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import kotlinx.cinterop.sizeOf
import kotlinx.cinterop.toKString
import org.maplibre.nativeffi.internal.c.mln_diagnostic

/**
 * Supplies the `mln_diagnostic` that each status-returning C call writes its message into.
 *
 * Each thread reuses one buffer. Native writes the message when a call exits and [check] reads it
 * right after on the same thread, so calls nested in callbacks during that call cannot clobber it.
 */
@OptIn(ExperimentalForeignApi::class)
internal object NativeDiagnostics {
  /** Calls native with this thread's diagnostic and throws the mapped exception on failure. */
  inline fun check(call: (CPointer<mln_diagnostic>) -> Int) {
    val diagnostic = buffer()
    Status.check(call(diagnostic)) { message(diagnostic) }
  }

  /** Returns this thread's diagnostic, sized for the next call. */
  fun buffer(): CPointer<mln_diagnostic> =
    threadBuffer.pointer.also { it.pointed.size = sizeOf<mln_diagnostic>().toUInt() }

  /** Copies the message that the last call through [diagnostic] wrote. */
  fun message(diagnostic: CPointer<mln_diagnostic>): String = diagnostic.pointed.message.toKString()
}

@OptIn(ExperimentalForeignApi::class, ExperimentalNativeApi::class)
private class DiagnosticBuffer {
  val pointer: CPointer<mln_diagnostic> = nativeHeap.alloc<mln_diagnostic>().ptr

  // Frees the buffer once its thread ends and drops the thread-local reference.
  @Suppress("unused") private val cleaner = createCleaner(pointer) { nativeHeap.free(it) }
}

@ThreadLocal private val threadBuffer = DiagnosticBuffer()
