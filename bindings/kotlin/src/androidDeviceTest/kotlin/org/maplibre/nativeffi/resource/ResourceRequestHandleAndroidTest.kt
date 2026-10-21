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
 * The JVM binding reclaims through a Cleaner; see ResourceRequestHandleJvmTest.
 */
class ResourceRequestHandleAndroidTest {
  @Test
  fun anUnreachableRequestIsReleasedNatively(): Unit = runSuspendTest {
    val claimed = CompletableDeferred<TestWeakReference>()
    val provider = denyingProvider { request, handle ->
      if (request.requestedUrl != STYLE_URL) return@denyingProvider null
      // Only the request's own cancel callback references it once the provider returns.
      handle.setCancelCallback(ResourceRequestCancelHandler { handle.close() })
      claimed.complete(TestWeakReference(handle))
      ResourceProviderDecision.HANDLE
    }
    withMap(provider = provider) {
      map.setStyleUrl(STYLE_URL).awaitWithin("the style command")
      val reference = claimed.awaitWithin("the provider to claim the request")
      assertTrue(awaitCollected(reference), "the request never became unreachable")
      // Cleanup releases the unanswered request and fails its style load.
      // Native schedules this event before retiring the handle.
      awaitMapEvent(RuntimeEventType.MAP_LOADING_FAILED)
    }
  }

  private companion object {
    const val STYLE_URL = "custom://unreachable-style.json"
  }
}
