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
internal class UnreachableActions private constructor(threadName: String) {
  private val queue = ReferenceQueue<Any>()

  /** Keeps registrations reachable so the collector can enqueue them. */
  private val pending = ConcurrentHashMap.newKeySet<Registration>()
  private val pendingLock = Any()

  init {
    Thread(::drain, threadName).apply { isDaemon = true }.start()
  }

  /** Runs [action] once [referent] becomes unreachable. */
  fun register(referent: Any, action: Runnable) {
    // Construct and retain the phantom reference under the same lock used by
    // the drain path. If collection enqueues it immediately, draining waits
    // until insertion finishes and cannot remove-before-add.
    synchronized(pendingLock) { pending.add(Registration(referent, queue, action)) }
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
        synchronized(pendingLock) { pending.remove(enqueued) }
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

  internal companion object {
    private val shared = UnreachableActions("maplibre-unreachable-actions")

    /** Runs [action] on the shared reclamation worker once [referent] becomes unreachable. */
    fun register(referent: Any, action: Runnable) {
      shared.register(referent, action)
    }

    /** Creates an independent worker for actions that may block. */
    fun isolated(threadName: String): UnreachableActions = UnreachableActions(threadName)
  }
}
