package org.maplibre.nativeffi

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
 * Runs [block], then releases this owner and suspends until the release finishes. An owner that
 * [block] already released or disposed is left as it is. A release failure propagates, or, when
 * [block] throws, is added to its throwable as suppressed, as with `use`.
 *
 * `use` can't suspend, so it calls `close`, which disposes the owner without waiting.
 */
public suspend inline fun <T : AsyncReleasable, R> T.useReleasing(block: suspend (T) -> R): R {
  var failure: Throwable? = null
  try {
    return block(this)
  } catch (thrown: Throwable) {
    failure = thrown
    throw thrown
  } finally {
    // A finally block also releases when [block] returns from the caller.
    if (!isClosed) {
      if (failure == null) {
        withContext(NonCancellable) { release().await() }
      } else {
        try {
          withContext(NonCancellable) { release().await() }
        } catch (releaseFailure: Throwable) {
          failure.addSuppressed(releaseFailure)
        }
      }
    }
  }
}
