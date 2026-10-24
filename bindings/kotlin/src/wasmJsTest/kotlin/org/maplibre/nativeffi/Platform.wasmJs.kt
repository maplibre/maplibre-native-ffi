@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi

import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.test.TestResult
import kotlinx.coroutines.test.runTest
import kotlinx.coroutines.withContext
import kotlinx.coroutines.withTimeoutOrNull
import kotlinx.coroutines.yield
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.LogEvent
import org.maplibre.nativeffi.generated.QueuedLogHandler
import org.maplibre.nativeffi.generated.QueuedResourceProvider
import org.maplibre.nativeffi.generated.RuntimeHandle
import org.maplibre.nativeffi.generated.logSetCallback
import org.maplibre.nativeffi.generated.setResourceProvider
import org.maplibre.nativeffi.internal.delivery.HostDelivery

/**
 * Where Karma serves the test module, the browser module with the fixtures of test_support.c; see
 * karma.config.d/browser.js.
 */
internal const val TEST_MODULE_URL: String =
  "/maplibre-native-kotlin/maplibre-native-kotlin-test.mjs"

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
      Maplibre.loadModule(TEST_MODULE_URL, TEST_POOL_SIZE)
      if (workersAtLoad == 0) workersAtLoad = TestSupport.allWorkers()
      block()
    }
  }

internal actual class TestWeakReference actual constructor(value: Any) {
  private val reference: JsAny = weakRef(value.toJsReference())

  actual val value: Any?
    get() = deref(reference)?.get()

  actual val isCleared: Boolean
    get() = deref(reference) == null
}

// Karma starts Chromium with --expose-gc, so each round runs a full collection.
internal actual suspend fun awaitCollected(reference: TestWeakReference): Boolean =
  withTimeoutOrNull(WAIT_TIMEOUT) {
    while (!reference.isCleared) {
      // The page keeps a weak reference's value until the task that read it ends.
      yield()
      collectGarbage()
    }
    true
  } ?: reference.isCleared

internal actual suspend fun requestCollection() {
  yield()
  collectGarbage()
}

// The page receives the binding's reports through globalThis.reportError, which this replaces
// until the restore, as the other platforms replace their uncaught exception handlers.
internal actual fun interceptUncaught(sink: (Throwable) -> Boolean): (() -> Unit)? {
  val previous = replaceReportError { failure -> sink(failure.get()) }
  return { restoreReportError(previous) }
}

// Native keeps each routed request and queues it, and the page runs the callback as it drains.
internal actual fun RuntimeHandle.setTestProvider(provider: TestProvider): Deferred<Unit> =
  setResourceProvider(
    QueuedResourceProvider(provider.routes) { request, handle ->
      provider.callback(request, handle)
    },
  )

// Native consumes each record and queues it, and the page runs the callback as it drains.
internal actual fun setTestLogCallback(callback: (LogEvent) -> Unit) {
  GeneratedApi.logSetCallback(QueuedLogHandler { _, event, _, _ -> callback(event) })
}

// The page drains what native has queued so far, then waits for a marker wake that the shim posts
// behind every wake and release posted before it.
internal actual suspend fun awaitDeliveries() {
  HostDelivery.drainForTesting()
  val delivered = CompletableDeferred<Unit>()
  TestWake { delivered.complete(Unit) }
    .use { marker ->
      TestSupport.wake(marker.address)
      TestSupport.release(marker.address)
      delivered.awaitWithin("the wakes that native posted before")
    }
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
