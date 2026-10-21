package org.maplibre.nativeffi.resource

import kotlin.test.Test
import kotlin.test.assertTrue
import kotlinx.coroutines.CompletableDeferred
import org.maplibre.nativeffi.TestWeakReference
import org.maplibre.nativeffi.awaitCollected
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.denyingProvider
import org.maplibre.nativeffi.generated.ResourceRequestCancelHandler
import org.maplibre.nativeffi.generated.RuntimeEventType
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.withMap

/**
 * The page's FinalizationRegistry reclamation of a request handle that became unreachable
 * unreleased, which runs as a page task of its own. The JVM binding reclaims through a Cleaner; see
 * ResourceRequestHandleJvmTest.
 */
class ResourceRequestHandleWasmTest {
  @Test
  fun anUnreachableRequestIsReleasedNatively() = runSuspendTest {
    val claimed = CompletableDeferred<TestWeakReference>()
    val provider = denyingProvider { request, handle ->
      if (request.requestedUrl != STYLE_URL) return@denyingProvider false
      // Only the request's own cancel callback references it once the provider returns.
      handle.setCancelCallback(ResourceRequestCancelHandler { handle.close() })
      claimed.complete(TestWeakReference(handle))
      true
    }
    withMap(provider = provider) {
      map.setStyleUrl(STYLE_URL).awaitWithin("the style command")
      val reference = claimed.awaitWithin("the provider to claim the request")
      assertTrue(awaitCollected(reference), "the request never became unreachable")
      // A FinalizationRegistry task releases the unanswered request, which fails the style it was
      // loading.
      awaitMapEvent(RuntimeEventType.MAP_LOADING_FAILED)
    }
  }

  private companion object {
    const val STYLE_URL = "custom://unreachable-style.json"
  }
}
