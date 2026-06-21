package org.maplibre.nativeffi.internal.lifecycle

import kotlin.concurrent.atomics.AtomicInt
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
  private val liveChildren = AtomicInt(0)

  fun requireLive() {
    when (releaseState.load()) {
      STATE_LIVE -> return
      STATE_RELEASING -> throw Status.invalidState("$typeName is currently releasing")
      else -> throw Status.released(typeName)
    }
  }

  fun isReleased(): Boolean = releaseState.load() == STATE_CLOSED

  fun address(): Long = address

  fun retainChild(): ChildRetention {
    while (true) {
      requireLive()
      val count = liveChildren.load()
      if (!liveChildren.compareAndSet(count, count + 1)) {
        continue
      }
      try {
        requireLive()
        return ChildRetention(this)
      } catch (error: Throwable) {
        releaseChild()
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
    val childCount = liveChildren.load()
    if (childCount > 0) {
      releaseState.store(STATE_LIVE)
      throw Status.liveChildren(typeName, childCount)
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

  private fun releaseChild() {
    while (true) {
      val count = liveChildren.load()
      if (count == 0 || liveChildren.compareAndSet(count, count - 1)) {
        return
      }
    }
  }

  internal class ChildRetention(private val owner: HandleStateCore) {
    private val released = AtomicInt(0)

    fun close() {
      if (released.compareAndSet(0, 1)) {
        owner.releaseChild()
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
