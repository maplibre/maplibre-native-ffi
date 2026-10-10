package org.maplibre.nativeffi.callback

import kotlin.test.Test
import kotlin.test.assertTrue
import kotlinx.coroutines.CompletableDeferred
import org.maplibre.nativeffi.TestWeakReference
import org.maplibre.nativeffi.awaitCollected
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.denyingProvider
import org.maplibre.nativeffi.generated.ResourceProvider
import org.maplibre.nativeffi.generated.ResourceProviderDecision
import org.maplibre.nativeffi.generated.ResourceRequestCancelHandler
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.withMap

/** Whether a registered callback keeps the handle it is registered on reachable. */
class CallbackLifetimeTest {
  @Test
  fun aCallbackCapturingItsReceiverDoesNotKeepItReachable(): Unit = runSuspendTest {
    val claimed = CompletableDeferred<TestWeakReference>()
    withMap(provider = selfCapturingProvider(claimed)) {
      map.setStyleUrl(STYLE_URL).awaitWithin("the style command")
      // Only the request's own cancel callback references the request, and native holds that
      // callback while the map keeps the request live.
      val request = claimed.awaitWithin("the provider to claim the request")
      assertTrue(
        awaitCollected(request),
        "a cancel callback kept the request it captures reachable",
      )
    }
  }

  private fun selfCapturingProvider(
    claimed: CompletableDeferred<TestWeakReference>
  ): ResourceProvider = denyingProvider { request, handle ->
    if (request.requestedUrl != STYLE_URL) return@denyingProvider null
    handle.setCancelCallback(ResourceRequestCancelHandler { handle.close() })
    claimed.complete(TestWeakReference(handle))
    ResourceProviderDecision.HANDLE
  }

  private companion object {
    const val STYLE_URL = "custom://self-capturing-style.json"
  }
}
