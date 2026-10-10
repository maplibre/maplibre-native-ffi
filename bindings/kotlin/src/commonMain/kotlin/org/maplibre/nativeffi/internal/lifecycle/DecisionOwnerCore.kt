package org.maplibre.nativeffi.internal.lifecycle

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.internal.status.Status

/**
 * Ownership state for a handle that a callback decides to keep or hand back to native.
 *
 * The callback's decision settles who releases the native handle: an accepted handle is released
 * once the host closes it and in-flight calls drain, and a handed-back handle is native's to
 * release.
 */
@OptIn(ExperimentalAtomicApi::class)
internal class DecisionOwnerCore(
  private val typeName: String,
  private val releaseNative: () -> Unit,
) : AutoCloseable {
  private val state = AtomicInt(0)
  private val completion = AtomicInt(COMPLETION_OPEN)
  private val decisionFinalized = AtomicInt(0)
  private val nativeReference = NativeReference(releaseNative)

  fun beginComplete(): CompletionOperation {
    if (!completion.compareAndSet(COMPLETION_OPEN, COMPLETION_RUNNING)) {
      throw Status.invalidState("$typeName is already completed")
    }
    return try {
      CompletionOperation(this, retainLive())
    } catch (error: Throwable) {
      completion.store(COMPLETION_OPEN)
      throw error
    }
  }

  /** Whether close or completion has marked this handle, even while borrows still drain. */
  val isClosed: Boolean
    get() = state.load() and CLOSED_FLAG != 0

  fun <T> withLiveHandle(block: () -> T): T {
    val borrow = retainLive()
    try {
      return block()
    } finally {
      borrow.close()
    }
  }

  override fun close() {
    markClosed()
  }

  fun finishDecision(decision: Decision): Decision {
    if (!decisionFinalized.compareAndSet(0, 1)) return Decision.ACCEPT
    return if (isClosed || completion.load() != COMPLETION_OPEN || decision == Decision.ACCEPT) {
      nativeReference.markProviderOwned()
      tryReleaseNative()
      Decision.ACCEPT
    } else {
      nativeReference.markNativeWillRelease()
      markClosed()
      Decision.PASS_THROUGH
    }
  }

  fun finishException(): Decision? {
    if (!decisionFinalized.compareAndSet(0, 1)) return Decision.ACCEPT
    return if (isClosed || completion.load() != COMPLETION_OPEN) {
      nativeReference.markProviderOwned()
      tryReleaseNative()
      Decision.ACCEPT
    } else {
      nativeReference.markNativeWillRelease()
      markClosed()
      null
    }
  }

  fun releaseIfOwned() {
    nativeReference.releaseIfOwned()
  }

  private fun retainLive(): Borrow {
    while (true) {
      val current = state.load()
      if (current and CLOSED_FLAG != 0) throw Status.closed(typeName)
      val active = current and ACTIVE_MASK
      check(active < ACTIVE_MASK) { "too many active $typeName operations" }
      if (state.compareAndSet(current, current + 1)) return Borrow(this)
    }
  }

  private fun releaseBorrow() {
    while (true) {
      val current = state.load()
      val active = current and ACTIVE_MASK
      check(active > 0) { "$typeName operation count underflow" }
      val next = current - 1
      if (state.compareAndSet(current, next)) {
        if (next and CLOSED_FLAG != 0 && next and ACTIVE_MASK == 0) tryReleaseNative()
        return
      }
    }
  }

  private fun markClosed() {
    while (true) {
      val current = state.load()
      if (current and CLOSED_FLAG != 0) {
        tryReleaseNative()
        return
      }
      val next = current or CLOSED_FLAG
      if (state.compareAndSet(current, next)) {
        if (next and ACTIVE_MASK == 0) tryReleaseNative()
        return
      }
    }
  }

  private fun tryReleaseNative() {
    val current = state.load()
    if (current and CLOSED_FLAG != 0 && current and ACTIVE_MASK == 0) {
      nativeReference.releaseIfOwned()
    }
  }

  internal class CompletionOperation(
    private val owner: DecisionOwnerCore,
    private val borrow: Borrow,
  ) : AutoCloseable {
    private val closed = AtomicInt(0)

    fun markNotReachedNative() {
      owner.completion.store(COMPLETION_OPEN)
    }

    fun markCompleted() {
      owner.completion.store(COMPLETION_DONE)
    }

    override fun close() {
      if (closed.compareAndSet(0, 1)) borrow.close()
    }
  }

  internal class Borrow(private val owner: DecisionOwnerCore) : AutoCloseable {
    private val closed = AtomicInt(0)

    override fun close() {
      if (closed.compareAndSet(0, 1)) owner.releaseBorrow()
    }
  }

  private class NativeReference(private val releaseNative: () -> Unit) {
    private val state = AtomicInt(STATE_PENDING)

    fun markProviderOwned() {
      state.compareAndSet(STATE_PENDING, STATE_PROVIDER_OWNED)
    }

    fun markNativeWillRelease() {
      state.store(STATE_RELEASE_ACCOUNTED)
    }

    fun releaseIfOwned() {
      if (state.compareAndSet(STATE_PROVIDER_OWNED, STATE_RELEASE_ACCOUNTED)) {
        releaseNative()
      }
    }

    private companion object {
      private const val STATE_PENDING = 0
      private const val STATE_PROVIDER_OWNED = 1
      private const val STATE_RELEASE_ACCOUNTED = 2
    }
  }

  /** A callback's decision about who owns the native handle. */
  internal enum class Decision {
    /** The host keeps the handle and releases it. */
    ACCEPT,

    /** Native keeps the handle and releases it. */
    PASS_THROUGH,
  }

  private companion object {
    private const val CLOSED_FLAG = Int.MIN_VALUE
    private const val ACTIVE_MASK = Int.MAX_VALUE
    private const val COMPLETION_OPEN = 0
    private const val COMPLETION_RUNNING = 1
    private const val COMPLETION_DONE = 2
  }
}
