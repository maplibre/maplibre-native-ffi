package org.maplibre.nativeffi

/** A native thread that starts running [block] at construction. */
internal expect class TestThread(block: () -> Unit) {
  /** Waits for the block to return, and rethrows what it threw. */
  fun join()
}

/** Runs [block] on a second native thread and waits for it to finish. */
internal fun runOnBackgroundThread(block: () -> Unit) {
  TestThread(block).join()
}

/** A weak reference that the garbage collector clears once nothing else reaches its value. */
internal expect class TestWeakReference(value: Any) {
  /** The value, or null once the collector has cleared it. */
  val value: Any?

  val isCleared: Boolean
}

/**
 * Collects garbage until [reference] clears, and returns whether it did before the wait timeout.
 *
 * Each round blocks on the collector itself: a reference queue on the JVM and Android, or a
 * synchronous collection on Kotlin/Native.
 */
internal expect fun awaitCollected(reference: TestWeakReference): Boolean

/** Counts the completions the platform's completion bridge has handed to native. */
internal expect fun pendingCompletionsForTesting(): Int
