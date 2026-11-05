package org.maplibre.nativeffi

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.JvmSynthetic

/**
 * The result of an asynchronous call, which the library completes once.
 *
 * A failed call completes it exceptionally, usually with a [MaplibreException]. Callers can't
 * complete or cancel it, so every caller that holds it sees the same result.
 *
 * On the JVM and Android it is a `java.util.concurrent.CompletableFuture`, whose methods that would
 * complete or cancel it, such as `complete`, `cancel`, and `orTimeout`, throw
 * `UnsupportedOperationException`. A stage derived from it, such as by `thenApply`, belongs to its
 * caller, who may complete or cancel it. It is a [Completion] too, except on Android before API 31,
 * where it is a plain `CompletableFuture`.
 *
 * A stage that isn't `*Async` runs on the thread that completes the result, which may be a native
 * thread that runs a callback. On such a thread, `join` and `get` throw `IllegalStateException`
 * instead of waiting for a [Completion] that isn't complete, because native may need that thread to
 * complete it.
 */
public expect class Completion<T> {
  /**
   * Suspends until the result completes, and returns it or throws its failure. Cancelling the
   * caller stops only its wait, not the result that other callers share.
   */
  public suspend fun await(): T

  /**
   * A new [Deferred] that completes with the result. Cancelling it cancels only that [Deferred].
   */
  public fun asDeferred(): Deferred<T>

  /** Completes the result with [value], unless it completed already. */
  @JvmSynthetic internal fun succeed(value: T)

  /** Completes the result with [failure], unless it completed already. */
  @JvmSynthetic internal fun fail(failure: Throwable)

  internal companion object {
    /** A result that [succeed] or [fail] completes later. */
    @JvmSynthetic internal fun <T> pending(): Completion<T>

    /** A result that has completed with [value]. */
    @JvmSynthetic internal fun <T> completed(value: T): Completion<T>
  }
}
