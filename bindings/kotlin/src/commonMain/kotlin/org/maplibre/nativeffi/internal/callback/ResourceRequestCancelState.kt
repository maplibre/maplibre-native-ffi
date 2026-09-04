package org.maplibre.nativeffi.internal.callback

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.AtomicLong
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.internal.lifecycle.WeakBox

/**
 * Platform-neutral cancel callback state for one resource request.
 *
 * The handle holds this state alone, and the registry reaches it weakly, so a host callback may
 * capture its own handle without keeping the handle reachable.
 */
@OptIn(ExperimentalAtomicApi::class)
internal class ResourceRequestCancelState(
  private val registration: ResourceRequestCancelRegistration
) {
  private val current = AtomicReference<(() -> Unit)?>(null)

  /** Returns the registry token for this state, registering it on first use. */
  fun token(): Long = registration.token(this)

  /** Stores the callback that the next cancellation runs, or null to run none. */
  fun store(callback: (() -> Unit)?) {
    current.store(callback)
  }

  /**
   * Runs the stored callback and contains host failures, so none of them unwinds into C.
   *
   * The registry runs this outside its lock, so the callback may complete or close the same
   * request.
   */
  fun dispatch() {
    val callback = current.load() ?: return
    try {
      callback()
    } catch (_: Throwable) {
      // A host failure inside a cancel callback stays contained: MapLibre already discarded the
      // request, and the binding has no caller to report it to.
    }
  }
}

/**
 * The registry token for one resource request, and the state's only tie to the release path.
 *
 * The release path holds this token holder instead of the state itself. Reaching the state from
 * there would reach the host callback, and a callback that captures its own handle would then keep
 * that handle reachable from the unreachable-handle cleanup that is meant to reclaim it.
 */
@OptIn(ExperimentalAtomicApi::class)
internal class ResourceRequestCancelRegistration {
  private val token = AtomicLong(UNREGISTERED)

  /** Returns the token that routes cancellations to [state], registering it on first use. */
  fun token(state: ResourceRequestCancelState): Long {
    val existing = token.load()
    if (existing != UNREGISTERED) return existing
    val fresh = ResourceRequestCancelRegistry.register(state)
    if (token.compareAndSet(UNREGISTERED, fresh)) return fresh
    ResourceRequestCancelRegistry.unregister(fresh)
    return token.load()
  }

  /**
   * Drops the registry token.
   *
   * The owner runs this once the C request can no longer reach the token: after the native release
   * returns, or once the request went back to MapLibre unhandled.
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
 * cancellation never depends on per-request native callback memory staying mapped.
 */
@OptIn(ExperimentalAtomicApi::class)
internal object ResourceRequestCancelRegistry {
  private val updateLock = AtomicInt(0)
  private val states = HashMap<Long, WeakBox<ResourceRequestCancelState>>()
  private var nextToken = 1L

  fun register(state: ResourceRequestCancelState): Long = withUpdateLock {
    val token = nextToken++
    states[token] = WeakBox(state)
    token
  }

  fun unregister(token: Long) {
    withUpdateLock { states.remove(token) }
  }

  /**
   * Runs the callback registered for a token.
   *
   * A token that no live state holds returns: the request went back to MapLibre, the binding
   * released it, or the host dropped the handle without closing it, and a callback the host can no
   * longer observe stays unrun.
   */
  fun dispatch(token: Long) {
    // The lookup holds the lock and the callback does not, because a host callback may close its
    // request, and closing unregisters the token.
    val state = withUpdateLock {
      val box = states[token]
      val state = box?.get()
      if (box != null && state == null) states.remove(token)
      state
    }
    state?.dispatch()
  }

  fun isRegisteredForTesting(token: Long): Boolean = withUpdateLock { states[token]?.get() != null }

  private inline fun <R> withUpdateLock(block: () -> R): R {
    while (!updateLock.compareAndSet(0, 1)) {
      // Spin briefly; cancel callback registration and cancellation are both infrequent.
    }
    try {
      return block()
    } finally {
      updateLock.store(0)
    }
  }
}
