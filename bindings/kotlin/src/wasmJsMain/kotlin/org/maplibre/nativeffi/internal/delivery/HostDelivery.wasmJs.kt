@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.internal.delivery

import org.maplibre.nativeffi.Status
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.c.Deliveries
import org.maplibre.nativeffi.internal.c.DeliveryBatchViewLayout
import org.maplibre.nativeffi.internal.c.DeliveryLayout
import org.maplibre.nativeffi.internal.c.Upcalls
import org.maplibre.nativeffi.internal.c.unsignedWord
import org.maplibre.nativeffi.internal.c.word
import org.maplibre.nativeffi.internal.call.ThreadStart
import org.maplibre.nativeffi.internal.callback.contain
import org.maplibre.nativeffi.internal.memory.NativeArena
import org.maplibre.nativeffi.internal.memory.readAddress
import org.maplibre.nativeffi.internal.memory.readI64
import org.maplibre.nativeffi.internal.memory.readU32
import org.maplibre.nativeffi.internal.status.NativeDiagnostics
import org.maplibre.nativeffi.internal.status.checkStatus

/**
 * The browser module's delivery queue, which every registration and completion names, and the
 * page's half of the module's wake shim.
 *
 * Native threads reach the page only through the shim in `mln_kw_runtime.c`, which posts each wake
 * as a page task that runs from the event loop, never inside a C call. Token 0 is the queue's own
 * wake, and the page drains the queue into [Deliveries]. Any other token is an `mln_wake`
 * registration's, posted once for each wake and once more for its release. A thread start posts its
 * call's status.
 */
internal actual object HostDelivery {
  private var installed = 0L

  actual val queue: Long
    get() = installed

  /** The function-table index of the shim's wake callback. */
  val wake: Long
    get() = unsignedWord(mln_kw_wake_function())

  /** The function-table index of the shim's wake release. */
  val wakeRelease: Long
    get() = unsignedWord(mln_kw_wake_release_function())

  /** Creates the module's queue, once, before any registration or completion names it. */
  fun install() {
    if (installed != 0L) return
    NativeArena().use { arena ->
      val out = arena.allocate(8)
      NativeDiagnostics.check { diagnostic -> mln_kw_install(word(out), word(diagnostic)) }
      installed = readI64(out)
    }
  }

  /** Handles one page task of the shim, of [kind] for [token]. */
  fun deliver(kind: Int, token: Int, status: Int) {
    contain("mln_wake_callback", Unit) {
      when (kind) {
        WAKE -> if (token == 0) drain() else Deliveries.wake(token.toLong())
        RELEASE -> Upcalls.releaseRoot(token.toLong())
        THREAD_START -> ThreadStart.deliver(token.toLong(), status)
        else -> error("the wake shim posted an unknown kind $kind")
      }
    }
  }

  /** Drains what native has queued so far, as the queue's next wake would. */
  fun drainForTesting() = drain()

  /**
   * Drains the queue once and runs each delivery's site in order. A delivery that arrives later
   * wakes the queue again.
   */
  private fun drain() {
    NativeArena().use { arena ->
      val out = arena.allocate(8)
      val view = arena.sized(DeliveryBatchViewLayout.SIZEOF, DeliveryBatchViewLayout.ALIGNOF)
      val diagnostic = NativeDiagnostics.buffer()
      val status = C.mln_delivery_queue_drain(queue, out, view, diagnostic)
      if (status == Status.NOT_READY.rawValue) return
      checkStatus(status) { NativeDiagnostics.message(diagnostic) }
      val batch = readI64(out)
      try {
        val stride = readU32(view + DeliveryBatchViewLayout.DELIVERY_SIZE).toLong()
        val first = readAddress(view + DeliveryBatchViewLayout.DELIVERIES)
        val count = readAddress(view + DeliveryBatchViewLayout.DELIVERY_COUNT)
        for (index in 0L until count) {
          val delivery = first + index * stride
          contain("mln_delivery", Unit) {
            Deliveries.dispatch(
              readU32(delivery + DeliveryLayout.TYPE),
              readAddress(delivery + DeliveryLayout.USER_DATA),
              delivery + DeliveryLayout.PAYLOAD,
            ) {
              NativeDiagnostics.check { C.mln_delivery_batch_take(batch, index, it) }
            }
          }
        }
      } finally {
        C.mln_delivery_batch_release(batch)
      }
    }
  }

  // The kinds of `mln_kw_runtime.c`.
  private const val WAKE = 1
  private const val RELEASE = 2
  private const val THREAD_START = 3
}

private fun mln_kw_install(queue: Int, diagnostic: Int): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_install(queue, diagnostic)")

private fun mln_kw_wake_function(): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_wake_function()")

private fun mln_kw_wake_release_function(): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]._mln_kw_wake_release_function()")
