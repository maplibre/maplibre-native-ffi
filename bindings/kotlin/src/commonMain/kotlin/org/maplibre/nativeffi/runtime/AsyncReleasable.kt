package org.maplibre.nativeffi.runtime

import kotlinx.coroutines.Deferred
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.withContext

/** An owner whose native release completes asynchronously. */
public interface AsyncReleasable {
  /**
   * Starts the native release; the result completes once native teardown has finished. Throws a
   * MaplibreException when native refuses the release, such as while a child is live, and leaves
   * the owner live for a retry. A release after native accepted one reports the handle released.
   */
  public fun release(): Deferred<Unit>

  /** Whether a release or dispose has ended the handle. */
  public val isClosed: Boolean
}

/**
 * Runs [block] and suspends until this owner's native release has finished. An owner that [block]
 * already closed is left as it is.
 */
public suspend inline fun <T : AsyncReleasable, R> T.use(block: suspend (T) -> R): R =
  try {
    block(this)
  } finally {
    if (!isClosed) withContext(NonCancellable) { release().await() }
  }
