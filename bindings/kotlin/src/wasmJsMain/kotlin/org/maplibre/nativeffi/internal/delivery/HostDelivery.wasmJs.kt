@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.internal.delivery

import org.maplibre.nativeffi.Status
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.c.word
import org.maplibre.nativeffi.internal.callback.contain
import org.maplibre.nativeffi.internal.memory.NativeArena
import org.maplibre.nativeffi.internal.memory.readI64
import org.maplibre.nativeffi.internal.status.NativeDiagnostics
import org.maplibre.nativeffi.internal.status.checkStatus

/**
 * The browser module's delivery queue, which every registration and completion names.
 *
 * Native threads reach the page only through the shim in `mln_kw_runtime.c`, which posts each wake
 * of the queue as a page task that runs from the event loop, never inside a C call. The task
 * dispatches the queue, which calls each site's trampoline on the page.
 */
internal actual object HostDelivery {
  private var installed = 0L

  actual val queue: Long
    get() = installed

  /** Creates the module's queue, once, before any registration or completion names it. */
  fun install() {
    if (installed != 0L) return
    NativeArena().use { arena ->
      val out = arena.allocate(8)
      NativeDiagnostics.check { diagnostic -> mln_kw_install(word(out), word(diagnostic)) }
      installed = readI64(out)
    }
  }

  /** Handles one wake of the queue: runs what native has queued so far. */
  fun wake() {
    contain("mln_wake_callback", Unit) { dispatch() }
  }

  /**
   * Runs what native has queued so far, as the queue's next wake would. A call queued during the
   * dispatch wakes the queue again.
   */
  fun dispatch() {
    val diagnostic = NativeDiagnostics.buffer()
    val status = C.mln_delivery_queue_dispatch(queue, diagnostic)
    if (status == Status.NOT_READY.rawValue) return
    checkStatus(status) { NativeDiagnostics.message(diagnostic) }
  }
}

private fun mln_kw_install(queue: Int, diagnostic: Int): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_install(queue, diagnostic)")
