package org.maplibre.nativeffi.examples.androidmap

import java.util.concurrent.CountDownLatch
import kotlin.coroutines.Continuation
import kotlin.coroutines.EmptyCoroutineContext
import kotlin.coroutines.startCoroutine

internal fun <T> runSuspend(block: suspend () -> T): T {
  val completed = CountDownLatch(1)
  var outcome: Result<T>? = null
  block.startCoroutine(
    object : Continuation<T> {
      override val context = EmptyCoroutineContext

      override fun resumeWith(result: Result<T>) {
        outcome = result
        completed.countDown()
      }
    }
  )
  completed.await()
  return outcome?.getOrThrow() ?: error("suspend call completed without a result")
}
