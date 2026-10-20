package org.maplibre.nativeffi.callback

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.concurrent.atomics.update
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import kotlinx.coroutines.CompletableDeferred
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.deny
import org.maplibre.nativeffi.denyingProvider
import org.maplibre.nativeffi.error.CallbackException
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.ResourceKind
import org.maplibre.nativeffi.generated.ResourceKindMask
import org.maplibre.nativeffi.generated.ResourceProvider
import org.maplibre.nativeffi.generated.ResourceProviderDecision
import org.maplibre.nativeffi.generated.ResourceRequestCancelHandler
import org.maplibre.nativeffi.generated.ResourceRequestHandle
import org.maplibre.nativeffi.generated.ResourceResponse
import org.maplibre.nativeffi.generated.ResourceResponseStatus
import org.maplibre.nativeffi.generated.ResourceRoute
import org.maplibre.nativeffi.generated.ResourceTransform
import org.maplibre.nativeffi.generated.ResourceTransformResponse
import org.maplibre.nativeffi.generated.RuntimeEventType
import org.maplibre.nativeffi.generated.RuntimeHandle
import org.maplibre.nativeffi.generated.UrlMatchFlags
import org.maplibre.nativeffi.interceptCallbackFailures
import org.maplibre.nativeffi.runOnBackgroundThread
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.serve
import org.maplibre.nativeffi.withMap

/**
 * The resource provider and transform callbacks: what they root, what they may call, and what they
 * hand out.
 */
@OptIn(ExperimentalAtomicApi::class)
class ResourceCallbackTest {

  @Test
  fun aCancelCallbackIsRootedUntilNativeReleasesIt(): Unit = runSuspendTest {
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
      assertFailsWith<InvalidStateException> {
        request.setCancelCallback(ResourceRequestCancelHandler {})
      }
      assertEquals(1, request.bindingCallbacks.rootCountForTesting())

      // Provider removal waits for the callback to return. Closing the unanswered request then
      // releases its cancel registration before the root count is read.
      runtime.clearResourceProvider().awaitWithin("provider removal")
      request.close()
      assertEquals(0, request.bindingCallbacks.rootCountForTesting())
      assertEquals(0, cancels.load())
    }
  }

  @Test
  fun aCancelRegistrationNativeDoesNotStoreIsNotRooted(): Unit = runSuspendTest {
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
  fun aForbiddenCallInsideACallbackFailsAndAnAllowedOneSucceeds(): Unit = runSuspendTest {
    val outcome = CompletableDeferred<List<Throwable?>>()
    lateinit var fixtureRuntime: RuntimeHandle
    val provider = denyingProvider { request, handle ->
      if (request.requestedUrl != STYLE_URL) return@denyingProvider null
      val results =
        listOf(
          runCatching { fixtureRuntime.clearResourceTransform() }.exceptionOrNull(),
          runCatching { fixtureRuntime.release() }.exceptionOrNull(),
          runCatching {
            handle.complete(
              ResourceResponse(
                ResourceResponseStatus.OK,
                bytes = EMPTY_STYLE_JSON.encodeToByteArray(),
              ),
            )
            handle.close()
          }
            .exceptionOrNull(),
        )
      outcome.complete(results)
      ResourceProviderDecision.HANDLE
    }
    withMap(provider = provider) {
      fixtureRuntime = runtime
      map.setStyleUrl(STYLE_URL).awaitWithin("the style command")
      val (command, release, completion) = outcome.awaitWithin("the provider callback")
      assertTrue(command is InvalidStateException, "command inside the provider: $command")
      assertTrue(release is InvalidStateException, "release inside the provider: $release")
      assertEquals(null, completion, "completing the provided request is allowed")
      assertFalse(runtime.isClosed)
      awaitMapEvent(RuntimeEventType.MAP_STYLE_LOADED)
    }
  }

  @Test
  fun aProviderThatThrowsFailsItsRequestAndLeavesTheRuntimeWorking(): Unit = runSuspendTest {
    val thrown = IllegalStateException("provider failure")
    val provider = denyingProvider { request, _ ->
      if (request.requestedUrl == STYLE_URL) throw thrown
      null
    }
    val reports = AtomicReference(emptyList<CallbackException>())
    val restore = interceptCallbackFailures { failure ->
      // Other tests' callbacks may fail concurrently, so only this exception counts.
      if (failure.cause === thrown) reports.update { it + failure }
    }
    try {
      withMap(provider = provider) {
        map.setStyleUrl(STYLE_URL).awaitWithin("the style command")
        // The binding contains the exception and answers native with a failed decision, which
        // MapLibre reports as the style's loading failure.
        awaitMapEvent(RuntimeEventType.MAP_LOADING_FAILED)
        loadStyle()
      }
    } finally {
      restore?.invoke()
    }
    // The report reaches the platform's handler before native receives the decision.
    if (restore != null) {
      assertEquals(listOf("mln_resource_provider_callback"), reports.load().map { it.callback })
    }
  }

  @Test
  fun theHandlerOfAContainedExceptionCannotCallNative(): Unit = runSuspendTest {
    val thrown = IllegalStateException("transform failure")
    val reentry = CompletableDeferred<Throwable?>()
    val restore =
      interceptCallbackFailures { failure ->
        // The handler runs on the transform's stack after its admission ends, where the binding
        // still refuses every native call.
        if (failure.cause === thrown)
          reentry.complete(runCatching { GeneratedApi.networkGetStatus() }.exceptionOrNull())
      } ?: return@runSuspendTest
    try {
      withMap(
        provider =
          denyingProvider { request, _ ->
            if (request.requestedUrl == STYLE_URL) ResourceProviderDecision.PASS_THROUGH else null
          },
      ) {
        runtime
          .setResourceTransform(
            ResourceTransform(callback = { _, url, _ -> if (url == STYLE_URL) throw thrown }),
          )
          .awaitWithin("the transform")
        map.setStyleUrl(STYLE_URL).awaitWithin("the style command")
        val refusal = reentry.awaitWithin("the transform's report")
        assertTrue(refusal is InvalidStateException, "a native call inside the handler: $refusal")
        runtime.clearResourceTransform().awaitWithin("the transform clear")
      }
    } finally {
      restore()
    }
  }

  @Test
  fun aClaimedRequestCanBeAnsweredAfterTheProviderReturns(): Unit = runSuspendTest {
    val claimed = CompletableDeferred<ResourceRequestHandle>()
    withMap(provider = claimingProvider(STYLE_URL, claimed)) {
      map.setStyleUrl(STYLE_URL).awaitWithin("the style command")
      val request = claimed.awaitWithin("the provider to claim the request")

      // A response the C API cannot represent is refused and leaves the request answerable.
      assertFailsWith<InvalidArgumentException> {
        request.complete(
          ResourceResponse(ResourceResponseStatus.ERROR, errorMessage = "bad\u0000message"),
        )
      }
      request.complete(
        ResourceResponse(ResourceResponseStatus.OK, bytes = EMPTY_STYLE_JSON.encodeToByteArray()),
      )
      // A request takes one response.
      assertFailsWith<InvalidStateException> {
        request.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
      }
      request.close()
      awaitMapEvent(RuntimeEventType.MAP_STYLE_LOADED)
    }
  }

  @Test
  fun aTransformResponseRefusesUseAfterItsCallbackAndFromAnotherThread(): Unit = runSuspendTest {
    val outcome = CompletableDeferred<Pair<Throwable?, Throwable?>>()
    var escaped: ResourceTransformResponse? = null
    val transform =
      ResourceTransform(
        callback = { _, url, response ->
          if (url != STYLE_URL || outcome.isCompleted) return@ResourceTransform
          escaped = response
          var fromAnotherThread: Throwable? = null
          runOnBackgroundThread {
            fromAnotherThread =
              runCatching { GeneratedApi.resourceTransformResponseSetUrl(response, REWRITTEN_URL) }
                .exceptionOrNull()
          }
          val inside = runCatching {
            GeneratedApi.resourceTransformResponseSetUrl(response, REWRITTEN_URL)
          }
            .exceptionOrNull()
          outcome.complete(inside to fromAnotherThread)
        },
      )
    // The provider passes the style through to the online source, which applies the transform.
    withMap(
      provider =
        denyingProvider { request, _ ->
          if (request.requestedUrl == STYLE_URL) ResourceProviderDecision.PASS_THROUGH else null
        },
    ) {
      runtime.setResourceTransform(transform).awaitWithin("the transform")
      map.setStyleUrl(STYLE_URL).awaitWithin("the style command")
      val (inside, fromAnotherThread) = outcome.awaitWithin("the transform callback")
      assertEquals(null, inside, "setting the URL inside the callback is allowed")
      assertTrue(
        fromAnotherThread is InvalidStateException,
        "the response from another thread: $fromAnotherThread",
      )
      assertFailsWith<InvalidStateException> {
        GeneratedApi.resourceTransformResponseSetUrl(escaped!!, REWRITTEN_URL)
      }
      runtime.clearResourceTransform().awaitWithin("the transform clear")
    }
  }

  @Test
  fun routesDecideWhichRequestsReachTheProviderAndAUrlOnlyRouteMatchesEveryKind(): Unit =
    runSuspendTest {
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
        ResourceProvider(
          callback = { request, handle ->
            seen.update { it + (request.kind to request.requestedUrl) }
            when (request.requestedUrl) {
              ROUTED_STYLE_URL -> handle.serve(ROUTED_STYLE_JSON)
              ROUTED_SOURCE_URL -> handle.serve(TILEJSON).also { sourceRequested.complete(Unit) }
              else -> handle.deny()
            }
          },
          routes = routes,
        )
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
      if (request.requestedUrl != url) return@denyingProvider null
      claimed.complete(handle)
      ResourceProviderDecision.HANDLE
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
