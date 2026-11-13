@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.internal.delivery

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.c.DispatchedUpcalls
import org.maplibre.nativeffi.internal.c.UpcallStubs
import org.maplibre.nativeffi.internal.callback.contain
import org.maplibre.nativeffi.internal.memory.NativeArena
import org.maplibre.nativeffi.internal.memory.readI64
import org.maplibre.nativeffi.internal.memory.writeAddress
import org.maplibre.nativeffi.internal.memory.writeU32
import org.maplibre.nativeffi.internal.status.NativeDiagnostics
import org.maplibre.nativeffi.internal.status.checkStatus

/**
 * The browser module's delivery queue, which every registration and completion names.
 *
 * The module runs on this thread, and wakes the queue from inside a call as often as from a task of
 * its own, so a wake only schedules a microtask that dispatches the queue once the call returns.
 * The dispatch calls each site's trampoline, which runs the site through [DispatchedUpcalls].
 */
internal actual object HostDelivery {
  private var installed = 0L
  private var scheduled = false

  actual val queue: Long
    get() = installed

  /** Creates the module's queue, once, before any registration or completion names it. */
  fun install() {
    if (installed != 0L) return
    NativeArena().use { arena ->
      // mln_delivery_queue_options on wasm32: its wake, an mln_wake whose 64-bit queue aligns it
      // to 8, holds the callback at 0 and the user data at 4. The queue's own wake has user data 0,
      // which no registration's root has.
      val options = arena.allocate(OPTIONS_SIZE)
      writeU32(options, OPTIONS_SIZE.toUInt())
      writeAddress(options + WAKE_CALLBACK, UpcallStubs.wakeCallback)
      val out = arena.allocate(8)
      NativeDiagnostics.check { diagnostic ->
        C.mln_delivery_queue_create(options, out, diagnostic)
      }
      installed = readI64(out)
    }
  }

  /** Runs a call of one of the module's trampolines. */
  fun upcall(name: String, arguments: JsAny) {
    if (name == WAKE && firstArgument(arguments) == 0) {
      schedule()
    } else {
      DispatchedUpcalls.run(name, arguments)
    }
  }

  private fun schedule() {
    if (scheduled) return
    scheduled = true
    queueMicrotask {
      scheduled = false
      contain(WAKE, Unit) { dispatch() }
    }
  }

  /**
   * Runs what native has queued so far, as the queue's next wake would. A call queued during the
   * dispatch wakes the queue again.
   */
  fun dispatch() {
    val diagnostic = NativeDiagnostics.buffer()
    val status = C.mln_delivery_queue_dispatch(queue, diagnostic)
    checkStatus(status) { NativeDiagnostics.message(diagnostic) }
  }

  private const val WAKE = "mln_wake_callback"
  private const val OPTIONS_SIZE = 40
  private const val WAKE_CALLBACK = 8L
}

private fun firstArgument(values: JsAny): Int = js("values[0] | 0")

private fun queueMicrotask(task: () -> Unit): Unit = js("queueMicrotask(task)")
