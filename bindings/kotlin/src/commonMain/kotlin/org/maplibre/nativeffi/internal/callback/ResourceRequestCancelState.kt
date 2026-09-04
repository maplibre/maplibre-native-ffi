package org.maplibre.nativeffi.internal.callback

import kotlin.concurrent.atomics.AtomicLong
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.lifecycle.PlatformLock
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
    return slot.exchange(null)
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
 * cancellation never dereferences per-request host memory.
 */
internal object ResourceRequestCancelRegistry {
  private val lock = PlatformLock()
  private val states = HashMap<Long, WeakBox<ResourceRequestCancelState>>()
  private var nextToken = 1L

  fun register(state: ResourceRequestCancelState): Long = lock.withLock {
    val token = nextToken++
    states[token] = WeakBox(state)
    token
  }

  fun unregister(token: Long) {
    lock.withLock { states.remove(token) }
  }

  /**
   * Runs the callback registered for a token, once.
   *
   * The lookup holds the registry lock and the callback does not, because the callback may close
   * its request and closing unregisters the token. A token without a live state is a request whose
   * handle the host dropped without closing, so its callback stays unrun.
   */
  fun dispatch(token: Long) {
    val state = lock.withLock { states.remove(token)?.get() }
    val callback = state?.take() ?: return
    ResourceRequestCancelState.runContained(callback)
  }

  fun isRegisteredForTesting(token: Long): Boolean = lock.withLock { states[token]?.get() != null }
}
