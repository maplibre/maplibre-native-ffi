package org.maplibre.nativeffi.internal.lifecycle

import java.lang.ref.PhantomReference
import java.lang.ref.ReferenceQueue
import java.util.concurrent.ConcurrentHashMap

/**
 * Runs registered actions once their referents become unreachable.
 *
 * Built on `PhantomReference` because `java.lang.ref.Cleaner` needs API 33 and the binding supports
 * API 24.
 *
 * A registered action must hold cleanup state only. An action that captures its referent keeps the
 * referent strongly reachable from this registry, so the action never runs.
 *
 * Actions run on one daemon thread, and a failing action does not stop later registrations.
 */
internal class UnreachableActions private constructor(threadName: String) {
  private val queue = ReferenceQueue<Any>()

  /** Keeps registrations reachable so the collector can enqueue them. */
  private val pending = ConcurrentHashMap.newKeySet<Registration>()
  private val pendingLock = Any()

  init {
    // The worker lives for the process, so pin the binding's class loader instead
    // of retaining the context of whichever thread registered first. The
    // constructor that also drops inherited thread locals needs API 33.
    Thread(::drain, threadName)
      .apply {
        isDaemon = true
        contextClassLoader = UnreachableActions::class.java.classLoader
      }
      .start()
  }

  /** Runs [action] once [referent] becomes unreachable. */
  fun register(referent: Any, action: Runnable) {
    // Shares the drain path's lock so an immediate enqueue cannot remove before add.
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
          // A failing action must not stop the drain loop.
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
