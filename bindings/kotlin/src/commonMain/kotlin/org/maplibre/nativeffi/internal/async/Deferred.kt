package org.maplibre.nativeffi.internal.async

import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.ExperimentalCoroutinesApi

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

/** A new [Deferred] that completes as [source] does, so cancelling it leaves [source] as it is. */
internal fun <T> copyOf(source: Deferred<T>): Deferred<T> {
  val copy = CompletableDeferred<T>()
  source.invokeOnCompletion { failure ->
    @OptIn(ExperimentalCoroutinesApi::class)
    if (failure == null) copy.complete(source.getCompleted())
    else copy.completeExceptionally(failure)
  }
  return copy
}
