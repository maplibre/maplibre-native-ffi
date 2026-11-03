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

internal fun runOnBackgroundThread(block: () -> Unit) {
  TestThread(block).join()
}

internal actual fun runSuspendTest(block: suspend CoroutineScope.() -> Unit): TestResult =
  runBlocking {
    withTimeout(TEST_TIMEOUT) { block() }
  }
