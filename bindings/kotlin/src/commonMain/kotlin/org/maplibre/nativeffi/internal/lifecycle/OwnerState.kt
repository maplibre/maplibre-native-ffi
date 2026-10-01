package org.maplibre.nativeffi.internal.lifecycle

import kotlinx.coroutines.Deferred

/**
 * The live-state contract a generated owner gives its operations.
 *
 * [HandleStateCore] implements it for plain owners and [DecisionOwnerState] for the owners a
 * provider callback decides about. A state never holds its owner, so the owner's cleanup can hold
 * the state.
 */
internal interface OwnerState {
  /** The native handle, after checking that the owner still holds it. */
  fun handle(): Long

  /** Runs [block] with the native handle while holding off a concurrent release. */
  fun <T> read(block: (Long) -> T): T

  /** The native handle whatever the owner's state, for calls native accepts at any time. */
  fun issued(): Long

  /** Runs the owner's synchronous release, the operation [name], once. */
  fun closeHandle(name: String, call: (Long) -> Unit)

  /** Runs the owner's asynchronous release once; later calls share its result. */
  fun retireHandle(call: (Long) -> Deferred<Unit>): Deferred<Unit>
}
