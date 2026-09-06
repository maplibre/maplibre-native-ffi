package org.maplibre.nativeffi.internal.callback

import kotlin.concurrent.atomics.AtomicLong
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.lifecycle.WeakBox
import org.maplibre.nativeffi.internal.status.Status

/** The outcome of the native set-cancel-callback call. */
internal class ResourceRequestCancelSetResult(val status: Int, val alreadyCancelled: Boolean)

/**
 * The cancel callback slot for one resource request.
 *
 * The request handle owns this state and the registry reaches it weakly, so a callback that
 * captures its own handle keeps nothing reachable from the registry. The slot hands its callback
 * out once, so the callback runs at most once no matter which thread takes it.
 */
@OptIn(ExperimentalAtomicApi::class)
internal class ResourceRequestCancelState(
  private val registration: ResourceRequestCancelRegistration
) {
  private val slot = AtomicReference<(() -> Unit)?>(null)

  /**
   * Stores [callback], registers a token, and calls [setNative] with it.
   *
   * The caller holds a live borrow on the handle and no lock. Returns the callback when native
   * reports that the request was already cancelled, so the caller runs it after the borrow ends. A
   * second registration and a failed native call throw and leave the slot empty.
   */
  fun register(
    callback: () -> Unit,
    setNative: (token: Long) -> ResourceRequestCancelSetResult,
  ): (() -> Unit)? {
    if (!slot.compareAndSet(null, callback)) {
      throw Status.invalidState("ResourceRequestHandle already has a cancel callback")
    }
    val token = registration.register(this)
    val result =
      try {
        setNative(token)
      } catch (error: Throwable) {
        rollBack(callback)
        throw error
      }
    if (result.status != MaplibreStatus.OK.nativeCode) {
      val failure = Status.exception(result.status)
      rollBack(callback)
      throw failure
    }
    if (!result.alreadyCancelled) return null
    registration.dispose()
    // A concurrent close may already have emptied the slot, which only stops a later dispatch. The
    // registration still hands its callback back to run once.
    slot.store(null)
    return callback
  }

  /** Takes the callback out of the slot, or returns null when it already ran or was dropped. */
  fun take(): (() -> Unit)? = slot.exchange(null)

  /** Drops the callback without running it. */
  fun drop() {
    slot.store(null)
  }

  private fun rollBack(callback: () -> Unit) {
    registration.dispose()
    slot.compareAndSet(callback, null)
  }

  companion object {
    /** Runs a host cancel callback and contains its failure, so nothing unwinds into C. */
    fun runContained(callback: () -> Unit) {
      try {
        callback()
      } catch (_: Throwable) {
        // MapLibre already discarded the request, and the binding has no caller to report to.
      }
    }
  }
}

/**
 * The registry token for one resource request.
 *
 * The release path holds this holder instead of the state, because the state reaches the host
 * callback and a callback that captures its own handle would then keep that handle reachable from
 * the unreachable-handle cleanup meant to reclaim it.
 */
@OptIn(ExperimentalAtomicApi::class)
internal class ResourceRequestCancelRegistration {
  private val token = AtomicLong(UNREGISTERED)

  fun register(state: ResourceRequestCancelState): Long {
    val fresh = ResourceRequestCancelRegistry.register(state)
    token.store(fresh)
    return fresh
  }

  /**
   * Drops the registry token. Idempotent.
   *
   * The owner runs this once C can no longer reach the token: after the native release returns,
   * after the request went back to MapLibre unhandled, or after a registration did not take.
   */
  fun dispose() {
    val existing = token.exchange(UNREGISTERED)
    if (existing != UNREGISTERED) ResourceRequestCancelRegistry.unregister(existing)
  }

  private companion object {
    private const val UNREGISTERED = 0L
  }
}

/**
 * Routes native resource request cancellations to the request that registered them.
 *
 * Platform bridges install one process-wide C callback and pass a token as its user data, so a
 * cancellation never dereferences per-request host memory. The table is a copy-on-write map behind
 * one atomic reference, so no lock is held on either side of the C boundary: it only ever holds the
 * requests with an open registration, and each update is one small copy.
 */
@OptIn(ExperimentalAtomicApi::class)
internal object ResourceRequestCancelRegistry {
  private val states = AtomicReference<Map<Long, WeakBox<ResourceRequestCancelState>>>(emptyMap())
  private val nextToken = AtomicLong(1L)

  fun register(state: ResourceRequestCancelState): Long {
    val token = nextToken.fetchAndAdd(1L)
    update { it + (token to WeakBox(state)) }
    return token
  }

  fun unregister(token: Long) {
    update { it - token }
  }

  /**
   * Runs the callback registered for a token, once.
   *
   * Removing the entry first makes the dispatch exclusive, and the callback then runs with nothing
   * held, because it may close its request and closing unregisters the token. A token without a
   * live state is a request whose handle the host dropped without closing, so its callback stays
   * unrun.
   */
  fun dispatch(token: Long) {
    var removed: WeakBox<ResourceRequestCancelState>? = null
    update { current ->
      removed = current[token]
      if (removed == null) current else current - token
    }
    val callback = removed?.get()?.take() ?: return
    ResourceRequestCancelState.runContained(callback)
  }

  fun isRegisteredForTesting(token: Long): Boolean = states.load()[token]?.get() != null

  private inline fun update(
    transform:
      (Map<Long, WeakBox<ResourceRequestCancelState>>) -> Map<
          Long,
          WeakBox<ResourceRequestCancelState>,
        >
  ) {
    while (true) {
      val current = states.load()
      val next = transform(current)
      if (next === current || states.compareAndSet(current, next)) return
    }
  }
}
