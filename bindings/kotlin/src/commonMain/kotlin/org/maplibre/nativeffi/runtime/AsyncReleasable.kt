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
 * already closed is left as it is. When [block] throws, its throwable propagates with any release
 * failure added as suppressed, as the standard library's `use` does. When [block] returns, a
 * release failure propagates.
 */
public suspend inline fun <T : AsyncReleasable, R> T.use(block: suspend (T) -> R): R {
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
