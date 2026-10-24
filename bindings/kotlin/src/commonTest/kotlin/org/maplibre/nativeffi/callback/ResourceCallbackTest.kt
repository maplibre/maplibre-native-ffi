package org.maplibre.nativeffi.callback

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.concurrent.atomics.update
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import kotlinx.coroutines.CompletableDeferred
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.NETWORK_URLS
import org.maplibre.nativeffi.assertFailsWithStatus
import org.maplibre.nativeffi.awaitDeliveries
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.deny
import org.maplibre.nativeffi.denyingProvider
import org.maplibre.nativeffi.error.CallbackException
import org.maplibre.nativeffi.generated.ResourceKind
import org.maplibre.nativeffi.generated.ResourceKindMask
import org.maplibre.nativeffi.generated.ResourceProvider
import org.maplibre.nativeffi.generated.ResourceRequestCancelHandler
import org.maplibre.nativeffi.generated.ResourceRequestHandle
import org.maplibre.nativeffi.generated.ResourceResponse
import org.maplibre.nativeffi.generated.ResourceResponseStatus
import org.maplibre.nativeffi.generated.ResourceRoute
import org.maplibre.nativeffi.generated.ResourceTransform
import org.maplibre.nativeffi.generated.ResourceTransformRequestHandle
import org.maplibre.nativeffi.generated.RuntimeEventType
import org.maplibre.nativeffi.generated.Status
import org.maplibre.nativeffi.generated.UrlMatchFlags
import org.maplibre.nativeffi.interceptUncaught
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.serve
import org.maplibre.nativeffi.withMap

/** The resource provider and transform callbacks: what they root and what they hand out. */
@OptIn(ExperimentalAtomicApi::class)
class ResourceCallbackTest {

  @Test
  fun aCancelCallbackIsRootedUntilNativeReleasesIt() = runSuspendTest {
    val claimed = CompletableDeferred<ResourceRequestHandle>()
    withMap(provider = claimingProvider(STYLE_URL, claimed)) {
      map.setStyleUrl(STYLE_URL).awaitWithin("the style command")
      val request = claimed.awaitWithin("the provider to claim the request")
      val cancels = AtomicInt(0)

      assertFalse(
        request.setCancelCallback(ResourceRequestCancelHandler { cancels.addAndFetch(1) }),
      )
      assertEquals(1, request.bindingCallbacks.rootCountForTesting())
      // Native keeps the first registration, and the rejected second one keeps no root.
      assertFailsWithStatus(Status.INVALID_STATE) {
        request.setCancelCallback(ResourceRequestCancelHandler {})
      }
      assertEquals(1, request.bindingCallbacks.rootCountForTesting())

      // Provider removal waits for the callback to return. Closing the unanswered request then
      // releases its cancel registration, which reaches Kotlin before the root count is read.
      runtime.clearResourceProvider().awaitWithin("provider removal")
      request.close()
      awaitDeliveries()
      assertEquals(0, request.bindingCallbacks.rootCountForTesting())
      assertEquals(0, cancels.load())
    }
  }

  @Test
  fun aCancelRegistrationNativeDoesNotStoreIsNotRooted() = runSuspendTest {
    val claimed = CompletableDeferred<ResourceRequestHandle>()
    val request =
      withMap(provider = claimingProvider(STYLE_URL, claimed)) {
        map.setStyleUrl(STYLE_URL).awaitWithin("the style command")
        claimed.awaitWithin("the provider to claim the request")
      }
    // The fixture has released the runtime, whose teardown cancelled the request. The request
    // outlives the runtime until the provider releases it.
    val cancels = AtomicInt(0)
    assertTrue(request.setCancelCallback(ResourceRequestCancelHandler { cancels.addAndFetch(1) }))
    assertEquals(0, request.bindingCallbacks.rootCountForTesting())
    assertEquals(0, cancels.load())
    request.close()
  }

  @Test
  fun aProviderThatThrowsFailsItsRequestAndLeavesTheRuntimeWorking() = runSuspendTest {
    val thrown = IllegalStateException("provider failure")
    val provider = denyingProvider { request, _ ->
      if (request.requestedUrl == STYLE_URL) throw thrown
      false
    }
    val reports = AtomicReference(emptyList<CallbackException>())
    val restore = interceptUncaught { failure ->
      // Other tests' callbacks may fail concurrently, so only this exception counts.
      (failure is CallbackException && failure.cause === thrown).also { taken ->
        if (taken) reports.update { it + failure as CallbackException }
      }
    }
    try {
      withMap(provider = provider) {
        map.setStyleUrl(STYLE_URL).awaitWithin("the style command")
        // The binding contains the exception and releases the request without a response, which
        // MapLibre reports as the style's loading failure.
        awaitMapEvent(RuntimeEventType.MAP_LOADING_FAILED)
        loadStyle()
      }
    } finally {
      restore?.invoke()
    }
    // The report reaches the platform's handler before the request fails.
    if (restore != null) {
      assertEquals(listOf("mln_resource_provider_callback"), reports.load().map { it.callback })
    }
  }

  @Test
  fun aClaimedRequestCanBeAnsweredAfterTheProviderReturns() = runSuspendTest {
    val claimed = CompletableDeferred<ResourceRequestHandle>()
    withMap(provider = claimingProvider(STYLE_URL, claimed)) {
      map.setStyleUrl(STYLE_URL).awaitWithin("the style command")
      val request = claimed.awaitWithin("the provider to claim the request")

      // A response the C API cannot represent is refused and leaves the request answerable.
      assertFailsWithStatus(Status.INVALID_ARGUMENT) {
        request.complete(
          ResourceResponse(ResourceResponseStatus.ERROR, errorMessage = "bad\u0000message"),
        )
      }
      request.complete(
        ResourceResponse(ResourceResponseStatus.OK, bytes = EMPTY_STYLE_JSON.encodeToByteArray()),
      )
      // The accepted response consumed the handle, so the owner has nothing left to release, and
      // native refuses a second answer on the stale handle.
      assertTrue(request.binding.isReleased)
      assertFailsWithStatus(Status.INVALID_STATE) {
        request.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
      }
      assertFailsWithStatus(Status.INVALID_STATE) { request.passThrough() }
      awaitMapEvent(RuntimeEventType.MAP_STYLE_LOADED)
    }
  }

  @Test
  fun aTransformRequestIsAnsweredAfterItsCallbackReturns() = runSuspendTest {
    val received = CompletableDeferred<ResourceTransformRequestHandle>()
    val transform =
      ResourceTransform(
        callback = { _, url, request ->
          if (url == STYLE_URL) received.complete(request) else request.close()
        },
      )
    // The provider claims only network requests, so the style reaches the online source, which
    // waits for the transform's answer. The test answers on its own thread, after the callback
    // returned on MapLibre's: another thread on a threaded target, and a later task on the page.
    withMap(provider = denyingProvider(routes = NETWORK_URLS)) {
      runtime.setResourceTransform(transform).awaitWithin("the transform")
      map.setStyleUrl(STYLE_URL).awaitWithin("the style command")
      val request = received.awaitWithin("the transform callback")
      request.complete(REWRITTEN_URL)
      request.close()
      assertFailsWithStatus(Status.INVALID_STATE) { request.complete(REWRITTEN_URL) }
      // The online source cannot load the rewritten scheme.
      awaitMapEvent(RuntimeEventType.MAP_LOADING_FAILED)
      runtime.clearResourceTransform().awaitWithin("the transform clear")
    }
  }

  @Test
  fun routesDecideWhichRequestsReachTheProviderAndAUrlOnlyRouteMatchesEveryKind() = runSuspendTest {
    val seen = AtomicReference(emptyList<Pair<ResourceKind, String?>>())
    val sourceRequested = CompletableDeferred<Unit>()
    val routes =
      mutableListOf(
        // Every URL, but only tiles, which this map never requests.
        ResourceRoute(kinds = ResourceKindMask.TILE, flags = UrlMatchFlags.GLOB, url = "**"),
        // Every kind under one prefix.
        ResourceRoute(flags = UrlMatchFlags.GLOB, url = "custom://routed/**"),
      )
    val provider =
      ResourceProvider(routes) { request, handle ->
        seen.update { it + (request.kind to request.requestedUrl) }
        when (request.requestedUrl) {
          ROUTED_STYLE_URL -> handle.serve(ROUTED_STYLE_JSON)
          ROUTED_SOURCE_URL -> handle.serve(TILEJSON).also { sourceRequested.complete(Unit) }
          else -> handle.deny()
        }
      }
    withMap {
      val installing = runtime.setResourceProvider(provider)
      routes.clear()
      installing.awaitWithin("the routed provider")

      // The style and the source it names are different kinds, and the URL-only route claims
      // both.
      map.setStyleUrl(ROUTED_STYLE_URL).awaitWithin("the routed style command")
      awaitMapEvent(RuntimeEventType.MAP_STYLE_LOADED)
      sourceRequested.awaitWithin("the routed source request")
      assertEquals(
        listOf(
          ResourceKind.STYLE to ROUTED_STYLE_URL,
          ResourceKind.SOURCE to ROUTED_SOURCE_URL,
        ),
        seen.load(),
      )

      // A style outside the prefix matches only the tile route's URL, so it passes through to
      // native loading, which serves no custom scheme. The provider decides before the load
      // fails, so the failure shows that the provider never saw the request.
      map.setStyleUrl(UNROUTED_STYLE_URL).awaitWithin("the unrouted style command")
      awaitMapEvent(RuntimeEventType.MAP_LOADING_FAILED)
      assertEquals(2, seen.load().size, "requests the provider saw: ${seen.load()}")
    }
  }

  /** Claims the request for [url] without answering it, and hands it to [claimed]. */
  private fun claimingProvider(url: String, claimed: CompletableDeferred<ResourceRequestHandle>) =
    denyingProvider { request, handle ->
      if (request.requestedUrl != url) return@denyingProvider false
      claimed.complete(handle)
      true
    }

  private companion object {
    const val STYLE_URL = "custom://style.json"
    const val REWRITTEN_URL = "unsupported://rewritten-style.json"
    const val ROUTED_STYLE_URL = "custom://routed/style.json"
    const val ROUTED_SOURCE_URL = "custom://routed/source.json"
    const val UNROUTED_STYLE_URL = "custom://unrouted/style.json"
    const val ROUTED_STYLE_JSON =
      """{"version":8,"sources":{"routed":{"type":"vector","url":"$ROUTED_SOURCE_URL"}},""" +
        """"layers":[]}"""
    const val TILEJSON = """{"tilejson":"2.2.0","tiles":["custom://routed/{z}/{x}/{y}.pbf"]}"""
  }
}
