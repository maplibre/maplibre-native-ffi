@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.test.TestResult
import kotlinx.coroutines.test.runTest
import kotlinx.coroutines.withContext
import kotlinx.coroutines.yield

/** The pool size the suite loads the module with, unlike the module's default of 20. */
internal const val TEST_POOL_SIZE: Int = 24

/** The Workers that the test module had as it loaded, before any test started a thread. */
internal var workersAtLoad: Int = 0
  private set

// The body runs on Dispatchers.Default rather than the test scheduler, so its waits use real time
// while native threads work. The module loads before the body, so a test that reaches native memory
// runs in runSuspendTest whatever order Karma runs the tests in.
internal actual fun runSuspendTest(block: suspend CoroutineScope.() -> Unit): TestResult =
  runTest(timeout = TEST_TIMEOUT) {
    withContext(Dispatchers.Default) {
      loadWasmModule(TEST_POOL_SIZE)
      if (workersAtLoad == 0) workersAtLoad = TestSupport.allWorkers()
      block()
    }
  }

internal actual class TestWeakReference actual constructor(value: Any) {
  private val reference: JsAny = weakRef(value.toJsReference())

  actual val value: Any?
    get() = deref(reference)?.get()
}

// Karma starts Chromium with --expose-gc, which defines globalThis.gc.
internal actual suspend fun requestCollection() {
  // The page keeps a weak reference's value until the task that read it ends.
  yield()
  collectGarbage()
}

// The page receives the binding's reports through globalThis.reportError, which this replaces
// until the restore, as the other platforms replace their uncaught exception handlers.
internal actual fun interceptUncaught(sink: (Throwable) -> Boolean): (() -> Unit)? {
  val previous = replaceReportError { failure -> sink(failure.get()) }
  return { restoreReportError(previous) }
}

private fun replaceReportError(sink: (JsReference<Throwable>) -> Boolean): JsAny =
  js(
    """{
      const previous = globalThis.reportError;
      globalThis.reportError = (error) => {
        const failure = error && error.mlnFailure;
        if (failure === undefined || !sink(failure)) previous.call(globalThis, error);
      };
      return previous;
    }""",
  )

private fun restoreReportError(previous: JsAny): Unit = js("globalThis.reportError = previous")

private fun weakRef(value: JsReference<Any>): JsAny = js("new WeakRef(value)")

private fun deref(reference: JsAny): JsReference<Any>? = js("reference.deref()")

private fun collectGarbage(): Unit = js("globalThis.gc()")
