package org.maplibre.nativeffi.runtime

import kotlinx.coroutines.Deferred
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.withContext

/** An owner whose native release completes asynchronously. */
public interface AsyncReleasable {
  /**
   * Starts the native release; the result completes once native teardown has finished. Throws the
   * mapped MaplibreException when native refuses the release, such as while a child is live, and
   * leaves the owner live for a retry. A repeated or concurrent call returns the same Deferred.
   */
  public fun release(): Deferred<Unit>
}

/** Runs [block] and suspends until this owner's native release has finished. */
public suspend inline fun <T : AsyncReleasable, R> T.use(block: suspend (T) -> R): R =
  try {
    block(this)
  } finally {
    withContext(NonCancellable) { release().await() }
  }
