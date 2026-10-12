package org.maplibre.nativeffi.resource

import kotlin.test.Test
import kotlin.test.assertTrue
import kotlinx.coroutines.CompletableDeferred
import org.maplibre.nativeffi.TestWeakReference
import org.maplibre.nativeffi.awaitCollected
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.denyingProvider
import org.maplibre.nativeffi.generated.ResourceProviderDecision
import org.maplibre.nativeffi.generated.ResourceRequestCancelHandler
import org.maplibre.nativeffi.generated.RuntimeEventType
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.withMap

/**
 * Android's phantom-reference reclamation of a request handle that became unreachable unreleased.
 * The JVM binding reclaims through a Cleaner instead; see ResourceRequestHandleJvmTest.
 */
class ResourceRequestHandleAndroidTest {
  @Test
  fun anUnreachableRequestIsReleasedNatively(): Unit = runSuspendTest {
    val claimed = CompletableDeferred<Pair<Long, TestWeakReference>>()
    val provider = denyingProvider { request, handle ->
      if (request.requestedUrl != STYLE_URL) return@denyingProvider null
      // Only the request's own cancel callback references it once the provider returns.
      handle.setCancelCallback(ResourceRequestCancelHandler { handle.close() })
      claimed.complete(handle.binding.issued() to TestWeakReference(handle))
      ResourceProviderDecision.HANDLE
    }
    withMap(provider = provider) {
      map.setStyleUrl(STYLE_URL).awaitWithin("the style command")
      val (raw, reference) = claimed.awaitWithin("the provider to claim the request")
      assertTrue(awaitCollected(reference), "the request never became unreachable")
      // Reclamation releases the unanswered request, which fails the style it was loading.
      awaitMapEvent(RuntimeEventType.MAP_LOADING_FAILED)
      assertTrue(requestIsReleased(raw), "the collected request is still live in native")
    }
  }

  private companion object {
    const val STYLE_URL = "custom://unreachable-style.json"
  }
}
