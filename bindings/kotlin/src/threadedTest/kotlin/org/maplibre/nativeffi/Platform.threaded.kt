package org.maplibre.nativeffi

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.test.TestResult
import kotlinx.coroutines.withTimeout
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.LogEvent
import org.maplibre.nativeffi.generated.LogHandler
import org.maplibre.nativeffi.generated.ResourceProvider
import org.maplibre.nativeffi.generated.ResourceProviderDecision
import org.maplibre.nativeffi.generated.RuntimeHandle
import org.maplibre.nativeffi.generated.logSetCallback
import org.maplibre.nativeffi.generated.setResourceProvider

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

internal actual fun holdThreadUntil(release: Deferred<Unit>) {
  runBlocking { release.await() }
}

// Native runs each release and callback inside its own call.
internal actual suspend fun awaitDeliveries() {}

internal actual fun RuntimeHandle.setTestProvider(provider: TestProvider): Deferred<Unit> =
  setResourceProvider(
    ResourceProvider(
      callback = { request, handle ->
        provider.callback(request, handle)
        ResourceProviderDecision.HANDLE
      },
      routes = provider.routes,
    ),
  )

internal actual fun setTestLogCallback(callback: (LogEvent) -> Unit) {
  GeneratedApi.logSetCallback(
    LogHandler { _, event, _, _ ->
      callback(event)
      1u
    },
  )
}
