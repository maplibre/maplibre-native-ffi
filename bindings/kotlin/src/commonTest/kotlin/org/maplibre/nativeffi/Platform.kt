package org.maplibre.nativeffi

import org.maplibre.nativeffi.error.CallbackException
import org.maplibre.nativeffi.internal.async.CompletionBridge

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

/**
 * Asks the collector for a full collection, which queues the cleaners of unreachable handles. The
 * cleaners run on their own worker, so a caller still waits for what they do.
 */
internal expect fun requestCollection()

/** Counts the completions the completion bridge has handed to native. */
internal fun pendingCompletionsForTesting(): Int = CompletionBridge.pendingCountForTesting()

/**
 * Routes each callback failure that the binding hands to the platform's handler to [sink], and
 * returns the function that restores the previous handler. Returns null on Android, where the
 * failure goes to the log, which a test cannot read back.
 */
internal expect fun interceptCallbackFailures(sink: (CallbackException) -> Unit): (() -> Unit)?
