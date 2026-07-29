package org.maplibre.nativeffi.internal.lifecycle

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.internal.status.Status

/** Platform-neutral release-state bookkeeping for native handles. */
@OptIn(ExperimentalAtomicApi::class)
internal class HandleStateCore(
  private val typeName: String,
  private val address: Long,
  vararg parents: Any,
) {
  @Suppress("unused") private val parents: Array<out Any> = parents
  val leakReport: LeakReport = LeakReport(typeName, address)
  private val releaseState = AtomicInt(STATE_LIVE)
  private val liveChildren = AtomicReference<List<String>>(emptyList())

  fun requireLive() {
    when (releaseState.load()) {
      STATE_LIVE -> return
      STATE_RELEASING -> throw Status.invalidState("$typeName is currently releasing")
      else -> throw Status.released(typeName)
    }
  }

  fun isReleased(): Boolean = releaseState.load() == STATE_CLOSED

  fun address(): Long = address

  /**
   * Retains this handle on behalf of a live child wrapper.
   *
   * [childTypeName] names the child wrapper type so that a blocked parent release can identify the
   * handles still holding it open.
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
    private val address: Long,
    private val writeLine: (String) -> Unit = { message -> println(message) },
  ) {
    private val released = AtomicInt(0)

    fun markReleased() {
      released.store(1)
    }

    fun report() {
      if (released.load() == 0) {
        writeLine(
          "Leaked $typeName native handle 0x${address.toString(16)}; " +
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
