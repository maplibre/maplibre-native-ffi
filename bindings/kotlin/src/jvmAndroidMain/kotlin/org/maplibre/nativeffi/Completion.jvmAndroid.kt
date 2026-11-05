package org.maplibre.nativeffi

import java.util.concurrent.CompletableFuture
import java.util.concurrent.CompletionException
import java.util.concurrent.ExecutionException
import java.util.concurrent.Executor
import java.util.concurrent.TimeUnit
import java.util.concurrent.TimeoutException
import java.util.concurrent.atomic.AtomicReference
import java.util.function.Supplier
import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.suspendCancellableCoroutine
import org.maplibre.nativeffi.internal.callback.DeliveryThread

public actual class Completion<T>
private constructor(
  // Whether the library completes this result. A derived stage belongs to its caller.
  private val shared: Boolean,
) : CompletableFuture<T>() {
  public actual suspend fun await(): T {
    if (isDone) {
      try {
        return super.join()
      } catch (failure: CompletionException) {
        throw cause(failure)
      }
    }
    return suspendCancellableCoroutine { waiter ->
      // Cancelling the waiter leaves the result as it is, and lets go of the waiter, so a result
      // that stays pending doesn't keep the cancelled coroutine.
      val pending = AtomicReference(waiter)
      waiter.invokeOnCancellation { pending.set(null) }
      whenComplete { value, failure ->
        val resumed = pending.getAndSet(null) ?: return@whenComplete
        if (failure == null) resumed.resume(value) else resumed.resumeWithException(cause(failure))
      }
    }
  }

  public actual fun asDeferred(): Deferred<T> {
    val deferred = CompletableDeferred<T>()
    whenComplete { value, failure ->
      if (failure == null) deferred.complete(value)
      else deferred.completeExceptionally(cause(failure))
    }
    return deferred
  }

  @JvmSynthetic
  internal actual fun succeed(value: T) {
    super.complete(value)
  }

  @JvmSynthetic
  internal actual fun fail(failure: Throwable) {
    super.completeExceptionally(failure)
  }

  override fun join(): T {
    checkWait()
    return super.join()
  }

  // Java needs the checked exceptions declared to catch them.
  @Throws(InterruptedException::class, ExecutionException::class)
  override fun get(): T {
    checkWait()
    return super.get()
  }

  @Throws(InterruptedException::class, ExecutionException::class, TimeoutException::class)
  override fun get(timeout: Long, unit: TimeUnit): T {
    checkWait()
    return super.get(timeout, unit)
  }

  override fun complete(value: T): Boolean {
    checkCaller()
    return super.complete(value)
  }

  override fun completeExceptionally(ex: Throwable): Boolean {
    checkCaller()
    return super.completeExceptionally(ex)
  }

  override fun cancel(mayInterruptIfRunning: Boolean): Boolean {
    checkCaller()
    return super.cancel(mayInterruptIfRunning)
  }

  override fun obtrudeValue(value: T) {
    checkCaller()
    super.obtrudeValue(value)
  }

  override fun obtrudeException(ex: Throwable) {
    checkCaller()
    super.obtrudeException(ex)
  }

  override fun completeAsync(supplier: Supplier<out T>, executor: Executor): CompletableFuture<T> {
    checkCaller()
    return super.completeAsync(supplier, executor)
  }

  override fun completeAsync(supplier: Supplier<out T>): CompletableFuture<T> {
    checkCaller()
    return super.completeAsync(supplier)
  }

  override fun completeOnTimeout(value: T, timeout: Long, unit: TimeUnit): CompletableFuture<T> {
    checkCaller()
    return super.completeOnTimeout(value, timeout, unit)
  }

  override fun orTimeout(timeout: Long, unit: TimeUnit): CompletableFuture<T> {
    checkCaller()
    return super.orTimeout(timeout, unit)
  }

  // Android calls this only from API 31.
  override fun <U> newIncompleteFuture(): CompletableFuture<U> = Completion(shared = false)

  private fun checkCaller() {
    if (shared) throw UnsupportedOperationException("only the library completes a Completion")
  }

  private fun checkWait() {
    check(isDone || !DeliveryThread.isCurrent) {
      "waiting on a thread that runs a native callback could deadlock: wait elsewhere, or chain a stage"
    }
  }

  internal actual companion object {
    @JvmSynthetic internal actual fun <T> pending(): Completion<T> = Completion(shared = true)

    @JvmSynthetic
    internal actual fun <T> completed(value: T): Completion<T> =
      pending<T>().also { it.succeed(value) }
  }
}

/** The failure that a stage reports, without the wrapper that a derived stage adds. */
private fun cause(failure: Throwable): Throwable =
  if (failure is CompletionException) failure.cause ?: failure else failure
