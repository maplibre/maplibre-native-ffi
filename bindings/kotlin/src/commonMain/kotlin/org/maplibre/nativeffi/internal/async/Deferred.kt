package org.maplibre.nativeffi.internal.async

import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.ExperimentalCoroutinesApi

/**
 * Wraps an eager deferred native handle in its public wrapper.
 *
 * The wrap runs as soon as the source completes, whether or not the caller still awaits the result,
 * so a cancelled await hands the wrapper to [closeDropped] instead of leaking the native handle it
 * owns.
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
