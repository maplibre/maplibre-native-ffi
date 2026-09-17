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
  transform: (T) -> R,
): Deferred<R> {
  val wrapped = CompletableDeferred<R>()
  invokeOnCompletion { failure ->
    if (failure != null) {
      wrapped.completeExceptionally(failure)
      return@invokeOnCompletion
    }
    val handle = transform(getCompleted())
    if (!wrapped.complete(handle)) closeDropped(handle)
  }
  return wrapped
}
