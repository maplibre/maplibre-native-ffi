package org.maplibre.nativeffi.internal.async

import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.ExperimentalCoroutinesApi

/**
 * Wraps an eager deferred native handle in its public wrapper.
 *
 * The transform runs when the source completes. If the caller cancelled the returned deferred
 * before completion, [closeDropped] releases the new wrapper.
 */
@OptIn(ExperimentalCoroutinesApi::class)
internal fun <T, R> Deferred<T>.mapHandleDeferred(
  closeDropped: (R) -> Unit,
  disposeUnadopted: (T) -> Unit,
  transform: (T) -> R,
): Deferred<R> {
  val wrapped = CompletableDeferred<R>()
  invokeOnCompletion { failure ->
    if (failure != null) {
      wrapped.completeExceptionally(failure)
      return@invokeOnCompletion
    }
    try {
      val raw = getCompleted()
      val handle = adoptOwned(raw, disposeUnadopted, transform)
      if (!wrapped.complete(handle)) closeDropped(handle)
    } catch (error: Throwable) {
      wrapped.completeExceptionally(error)
    }
  }
  return wrapped
}

/** Transfers a native result to its wrapper, releasing it if wrapper construction fails. */
internal fun <T, R> adoptOwned(value: T, dispose: (T) -> Unit, adopt: (T) -> R): R =
  try {
    adopt(value)
  } catch (failure: Throwable) {
    try {
      dispose(value)
    } catch (cleanup: Throwable) {
      failure.addSuppressed(cleanup)
    }
    throw failure
  }
