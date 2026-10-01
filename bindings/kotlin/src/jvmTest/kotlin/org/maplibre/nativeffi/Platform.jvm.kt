package org.maplibre.nativeffi

import java.lang.ref.ReferenceQueue
import java.lang.ref.WeakReference

internal actual class TestThread actual constructor(block: () -> Unit) {
  @Volatile private var failure: Throwable? = null
  private val thread =
    Thread(
        {
          try {
            block()
          } catch (error: Throwable) {
            failure = error
          }
        },
        "maplibre-test-background",
      )
      .apply { start() }

  actual fun join() {
    thread.join()
    failure?.let { throw it }
  }
}

internal actual class TestWeakReference actual constructor(value: Any) {
  val queue = ReferenceQueue<Any>()
  private val reference = WeakReference(value, queue)

  actual val value: Any?
    get() = reference.get()

  actual val isCleared: Boolean
    get() = reference.get() == null
}

internal actual fun awaitCollected(reference: TestWeakReference): Boolean {
  val deadline = System.nanoTime() + GC_WAIT_NANOS
  while (System.nanoTime() < deadline) {
    if (reference.isCleared) return true
    System.gc()
    if (reference.queue.remove(GC_ROUND_MILLIS) != null) return true
  }
  return reference.isCleared
}

internal actual fun requestCollection() {
  System.gc()
}

private const val GC_WAIT_NANOS = 10_000_000_000L
private const val GC_ROUND_MILLIS = 100L
