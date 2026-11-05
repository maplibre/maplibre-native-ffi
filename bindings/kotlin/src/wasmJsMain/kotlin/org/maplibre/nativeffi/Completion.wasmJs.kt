package org.maplibre.nativeffi

import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.async.copyOf

public actual class Completion<T> private constructor() {
  private val result = CompletableDeferred<T>()

  public actual suspend fun await(): T = result.await()

  public actual fun asDeferred(): Deferred<T> = copyOf(result)

  internal actual fun succeed(value: T) {
    result.complete(value)
  }

  internal actual fun fail(failure: Throwable) {
    result.completeExceptionally(failure)
  }

  internal actual companion object {
    internal actual fun <T> pending(): Completion<T> = Completion()

    internal actual fun <T> completed(value: T): Completion<T> =
      Completion<T>().also { it.succeed(value) }
  }
}
