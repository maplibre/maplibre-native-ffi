package org.maplibre.nativeffi.lifecycle

import kotlin.test.Test
import kotlin.test.assertTrue
import kotlin.time.Duration.Companion.milliseconds
import kotlin.time.TimeSource
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.withTimeoutOrNull
import org.maplibre.nativeffi.RuntimeFixture
import org.maplibre.nativeffi.WAIT_TIMEOUT
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.denyingProvider
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.ResourceProviderDecision
import org.maplibre.nativeffi.generated.ResourceRequestHandle
import org.maplibre.nativeffi.generated.RuntimeHandle
import org.maplibre.nativeffi.requestCollection
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.smallMapOptions

/** What happens to a handle that its owner drops without closing. */
class AbandonedHandleTest {
  @Test
  fun anAbandonedMapIsDisposedAndDropsItsPendingRequest(): Unit = runSuspendTest {
    abandonAMapAndAwaitItsDisposal()
  }
}

/**
 * Drops the last reference to a map whose style request a provider holds, and returns once the
 * collector's cleaner has disposed the map, which cancels the request. Disposal happens on the
 * cleaner's worker, never on a callback stack.
 */
internal suspend fun abandonAMapAndAwaitItsDisposal() {
  val claimed = CompletableDeferred<ResourceRequestHandle>()
  val provider = denyingProvider { request, handle ->
    if (request.requestedUrl != ABANDONED_STYLE_URL) return@denyingProvider null
    claimed.complete(handle)
    ResourceProviderDecision.HANDLE
  }
  RuntimeFixture.use(provider) {
    val holder = MapHolder()
    startStyleLoad(runtime, holder)
    val request = claimed.awaitWithin("the provider to claim the style request")
    try {
      val cancelled = CompletableDeferred<Unit>()
      if (request.setCancelCallback { cancelled.complete(Unit) }) {
        cancelled.complete(Unit)
      }
      holder.map = null
      awaitWhileCollecting("the abandoned map's disposal to cancel its request", cancelled)
      assertTrue(request.isCancelled())
    } finally {
      request.close()
    }
  }
}

/** Keeps the map reachable only until the test drops it. */
private class MapHolder {
  var map: MapHandle? = null
}

/** Creates a map in [holder] and starts its style load, keeping no other reference to the map. */
private suspend fun startStyleLoad(runtime: RuntimeHandle, holder: MapHolder) {
  val map = runtime.createMap(smallMapOptions()).awaitWithin("the map")
  holder.map = map
  map.setStyleUrl(ABANDONED_STYLE_URL).awaitWithin("the style command")
}

/** Requests collections until [done] completes, waiting on it between requests. */
private suspend fun awaitWhileCollecting(what: String, done: Deferred<Unit>) {
  val deadline = TimeSource.Monotonic.markNow() + WAIT_TIMEOUT
  while (deadline.hasNotPassedNow()) {
    requestCollection()
    if (withTimeoutOrNull(COLLECTION_ROUND) { done.await() } != null) return
  }
  throw AssertionError("timed out waiting for $what")
}

private const val ABANDONED_STYLE_URL = "custom://abandoned-style.json"
private val COLLECTION_ROUND = 100.milliseconds
