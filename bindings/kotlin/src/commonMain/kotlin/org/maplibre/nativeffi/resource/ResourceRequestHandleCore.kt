package org.maplibre.nativeffi.resource

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.status.Status

/** Platform-neutral ownership state for provider-owned resource request handles. */
@OptIn(ExperimentalAtomicApi::class)
internal class ResourceRequestHandleCore(private val releaseNative: () -> Unit) : AutoCloseable {
  private val state = AtomicInt(0)
  private val completion = AtomicInt(COMPLETION_OPEN)
  private val decisionFinalized = AtomicInt(0)
  private val nativeReference = NativeReference(releaseNative)

  fun beginComplete(): CompletionOperation {
    if (!completion.compareAndSet(COMPLETION_OPEN, COMPLETION_RUNNING)) {
      throw InvalidStateException(
        MaplibreStatus.INVALID_STATE.nativeCode,
        "ResourceRequestHandle is already completed",
      )
    }
    return try {
      CompletionOperation(this, retainLive())
    } catch (error: Throwable) {
      completion.store(COMPLETION_OPEN)
      throw error
    }
  }

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

  fun finishProviderDecision(decision: ResourceProviderDecision): ResourceProviderDecision {
    if (!decisionFinalized.compareAndSet(0, 1)) return ResourceProviderDecision.HANDLE
    return if (
      completion.load() != COMPLETION_OPEN || decision == ResourceProviderDecision.HANDLE
    ) {
      nativeReference.markProviderOwned()
      tryReleaseNative()
      ResourceProviderDecision.HANDLE
    } else {
      nativeReference.markNativeWillRelease()
      markClosed()
      ResourceProviderDecision.PASS_THROUGH
    }
  }

  fun finishProviderException(): ResourceProviderDecision? {
    if (!decisionFinalized.compareAndSet(0, 1)) return ResourceProviderDecision.HANDLE
    return if (completion.load() != COMPLETION_OPEN) {
      nativeReference.markProviderOwned()
      tryReleaseNative()
      ResourceProviderDecision.HANDLE
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
      if (current and CLOSED_FLAG != 0) throw Status.released("ResourceRequestHandle")
      val active = current and ACTIVE_MASK
      check(active < ACTIVE_MASK) { "too many active ResourceRequestHandle operations" }
      if (state.compareAndSet(current, current + 1)) return Borrow(this)
    }
  }

  private fun releaseBorrow() {
    while (true) {
      val current = state.load()
      val active = current and ACTIVE_MASK
      check(active > 0) { "ResourceRequestHandle operation count underflow" }
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
    private val owner: ResourceRequestHandleCore,
    private val borrow: Borrow,
  ) : AutoCloseable {
    private val closed = AtomicInt(0)

    fun markNotReachedNative() {
      owner.completion.store(COMPLETION_OPEN)
    }

    fun markCompleted() {
      owner.completion.store(COMPLETION_DONE)
      owner.markClosed()
    }

    override fun close() {
      if (closed.compareAndSet(0, 1)) borrow.close()
    }
  }

  internal class Borrow(private val owner: ResourceRequestHandleCore) : AutoCloseable {
    private val closed = AtomicInt(0)

    override fun close() {
      if (closed.compareAndSet(0, 1)) owner.releaseBorrow()
    }
  }

  private class NativeReference(private val releaseNative: () -> Unit) {
    private val state = AtomicInt(STATE_PENDING)

    fun markProviderOwned() {
      while (true) {
        when (state.load()) {
          STATE_PENDING -> if (state.compareAndSet(STATE_PENDING, STATE_PROVIDER_OWNED)) return
          else -> return
        }
      }
    }

    fun markNativeWillRelease() {
      while (true) {
        val current = state.load()
        if (current == STATE_RELEASE_ACCOUNTED) return
        if (state.compareAndSet(current, STATE_RELEASE_ACCOUNTED)) return
      }
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

  private companion object {
    private const val CLOSED_FLAG = Int.MIN_VALUE
    private const val ACTIVE_MASK = Int.MAX_VALUE
    private const val COMPLETION_OPEN = 0
    private const val COMPLETION_RUNNING = 1
    private const val COMPLETION_DONE = 2
  }
}
