package org.maplibre.nativeffi.internal.async

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async

/** Transforms an eager deferred value without imposing a caller scope. */
internal fun <T, R> Deferred<T>.mapDeferred(transform: (T) -> R): Deferred<R> =
  CoroutineScope(Dispatchers.Unconfined).async(start = CoroutineStart.UNDISPATCHED) {
    transform(await())
  }
