package org.maplibre.nativeffi

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.test.TestResult
import kotlinx.coroutines.withTimeout

/** A native thread that starts running [block] at construction. */
internal expect class TestThread(block: () -> Unit) {
  /** Waits for the block to return, and rethrows what it threw. */
  fun join()
}

/** Runs [block] on a second native thread and waits for it to finish. */
internal fun runOnBackgroundThread(block: () -> Unit) {
  TestThread(block).join()
}

internal actual fun runSuspendTest(block: suspend CoroutineScope.() -> Unit): TestResult =
  runBlocking {
    withTimeout(TEST_TIMEOUT) { block() }
  }

// Native runs each release and callback inside its own call.
internal actual suspend fun awaitDeliveries() {}

internal actual fun setTestLogCallback(callback: (LogEvent) -> Unit) {
  setLogCallback(
    LogHandler { _, event, _, _ ->
      callback(event)
      true
    },
  )
}
