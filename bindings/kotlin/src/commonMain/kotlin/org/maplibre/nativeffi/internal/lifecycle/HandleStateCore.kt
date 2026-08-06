package org.maplibre.nativeffi.internal.lifecycle

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.internal.status.Status

/** Platform-neutral release-state bookkeeping for native handles. */
@OptIn(ExperimentalAtomicApi::class)
internal class HandleStateCore(
  private val typeName: String,
  private val handleId: Long,
  vararg parents: Any,
) {
  @Suppress("unused") private val parents: Array<out Any> = parents
  val leakReport: LeakReport = LeakReport(typeName, handleId)
  private val releaseState = AtomicInt(STATE_LIVE)
  private val liveChildren = AtomicReference<List<String>>(emptyList())
  private val activeUses = AtomicInt(0)

  fun requireLive() {
    when (releaseState.load()) {
      STATE_LIVE -> return
      STATE_RELEASING -> throw Status.invalidState("$typeName is currently releasing")
      else -> throw Status.released(typeName)
    }
  }

  /**
   * Runs [block] with release held off until it returns, for handles the host may use and release
   * from different threads. Calling [closeOnce] from inside [block] deadlocks.
   */
  fun <T> withLive(block: () -> T): T {
    addActiveUse(1)
    try {
      requireLive()
      return block()
    } finally {
      addActiveUse(-1)
    }
  }

  private fun addActiveUse(delta: Int) {
    while (true) {
      val current = activeUses.load()
      if (activeUses.compareAndSet(current, current + delta)) return
    }
  }

  fun isReleased(): Boolean = releaseState.load() == STATE_CLOSED

  /** The C API handle id this wrapper owns. */
  fun handleId(): Long = handleId

  /**
   * Retains this handle on behalf of a live child wrapper. [childTypeName] appears in the error a
   * blocked parent release throws.
   */
  fun retainChild(childTypeName: String): ChildRetention {
    while (true) {
      requireLive()
      val children = liveChildren.load()
      if (!liveChildren.compareAndSet(children, children + childTypeName)) {
        continue
      }
      try {
        requireLive()
        return ChildRetention(this, childTypeName)
      } catch (error: Throwable) {
        releaseChild(childTypeName)
        throw error
      }
    }
  }

  /**
   * Destroys this handle once, and runs [afterSuccess] for the bookkeeping the destroy released.
   *
   * A failing [destroy] leaves the handle live and closable again, because the C API refused and
   * the native handle is still there. [afterSuccess] is the opposite: it runs after the handle has
   * been marked closed, since the native handle is gone by then and a wrapper that still called
   * itself live would offer calls that could only fail. A later `close` therefore returns without
   * reaching it.
   *
   * So **[afterSuccess] gets one attempt**, and what it does not finish is not finished by anyone.
   * A body of it that can fail -- releasing a callback registration, on a target where that waits
   * for a body already inside it -- has to make sure that the accounting the rest of the binding
   * depends on happens anyway, rather than leaving a parent retained by a handle that no longer
   * exists.
   */
  fun closeOnce(destroy: () -> Int, afterSuccess: () -> Unit = {}) {
    if (!releaseState.compareAndSet(STATE_LIVE, STATE_RELEASING)) {
      when (releaseState.load()) {
        STATE_CLOSED -> return
        STATE_RELEASING -> throw Status.invalidState("$typeName is currently releasing")
        else -> throw Status.released(typeName)
      }
    }
    val children = liveChildren.load()
    if (children.isNotEmpty()) {
      releaseState.store(STATE_LIVE)
      throw Status.liveChildren(typeName, children)
    }
    // Uses that already passed their liveness check still hold the handle; wait them out.
    while (activeUses.load() != 0) {
      yieldWhileClosing()
    }
    try {
      Status.check(destroy())
    } catch (error: Throwable) {
      releaseState.store(STATE_LIVE)
      throw error
    }
    leakReport.markReleased()
    releaseState.store(STATE_CLOSED)
    afterSuccess()
  }

  private fun releaseChild(childTypeName: String) {
    while (true) {
      val children = liveChildren.load()
      val index = children.indexOf(childTypeName)
      if (index < 0) {
        return
      }
      val remaining = children.toMutableList().apply { removeAt(index) }
      if (liveChildren.compareAndSet(children, remaining)) {
        return
      }
    }
  }

  /** One child wrapper's retention of its parent handle. Releasing more than once is a no-op. */
  internal class ChildRetention(
    private val owner: HandleStateCore,
    private val childTypeName: String,
  ) {
    private val released = AtomicInt(0)

    fun close() {
      if (released.compareAndSet(0, 1)) {
        owner.releaseChild(childTypeName)
      }
    }
  }

  @OptIn(ExperimentalAtomicApi::class)
  internal class LeakReport(
    private val typeName: String,
    private val handleId: Long,
    private val writeLine: (String) -> Unit = { message -> println(message) },
  ) {
    private val released = AtomicInt(0)

    fun markReleased() {
      released.store(1)
    }

    fun report() {
      if (released.load() == 0) {
        writeLine(
          "Leaked $typeName native handle 0x${handleId.toString(16)}; " +
            "close handles explicitly on their owner thread."
        )
      }
    }
  }

  private companion object {
    private const val STATE_LIVE = 0
    private const val STATE_RELEASING = 1
    private const val STATE_CLOSED = 2
  }
}
