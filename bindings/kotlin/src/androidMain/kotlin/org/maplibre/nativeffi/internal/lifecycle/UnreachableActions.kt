package org.maplibre.nativeffi.internal.lifecycle

import java.lang.ref.PhantomReference
import java.lang.ref.ReferenceQueue
import java.util.concurrent.ConcurrentHashMap

/**
 * Runs registered actions once their referents become unreachable.
 *
 * The JVM source set builds this on `java.lang.ref.Cleaner`. Android exposes that class from API 33
 * while this binding supports API 24, so Android builds the same contract from `PhantomReference`
 * and `ReferenceQueue`, which cover the whole supported range.
 *
 * A registered action holds cleanup state only. An action that captures its referent keeps the
 * referent strongly reachable from this registry, so the registration stays pending and the action
 * never runs.
 *
 * Actions run on one daemon thread. An action that fails leaves the thread draining later
 * registrations, matching `Cleaner`.
 */
internal object UnreachableActions {
  private val queue = ReferenceQueue<Any>()

  /** Keeps registrations reachable so the collector can enqueue them. */
  private val pending = ConcurrentHashMap.newKeySet<Registration>()

  init {
    Thread(::drain, "maplibre-unreachable-actions").apply { isDaemon = true }.start()
  }

  /** Runs [action] once [referent] becomes unreachable. */
  fun register(referent: Any, action: Runnable) {
    pending.add(Registration(referent, queue, action))
  }

  private fun drain() {
    while (true) {
      val enqueued =
        try {
          queue.remove()
        } catch (_: InterruptedException) {
          continue
        }
      if (enqueued is Registration) {
        pending.remove(enqueued)
        try {
          enqueued.run()
        } catch (_: Throwable) {
          // A failing action stays local to its own registration so later ones still run.
        }
      }
    }
  }

  private class Registration(
    referent: Any,
    queue: ReferenceQueue<Any>,
    private val action: Runnable,
  ) : PhantomReference<Any>(referent, queue) {
    fun run() {
      action.run()
    }
  }
}
