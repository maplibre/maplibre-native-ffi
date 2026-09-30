package org.maplibre.nativeffi.callback

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import kotlinx.coroutines.CompletableDeferred
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.denyingProvider
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.ResourceProviderDecision
import org.maplibre.nativeffi.generated.ResourceRequestHandle
import org.maplibre.nativeffi.generated.ResourceResponse
import org.maplibre.nativeffi.generated.ResourceResponseStatus
import org.maplibre.nativeffi.generated.ResourceTransform
import org.maplibre.nativeffi.generated.ResourceTransformResponse
import org.maplibre.nativeffi.generated.RuntimeEventType
import org.maplibre.nativeffi.runOnBackgroundThread
import org.maplibre.nativeffi.runSuspendTest
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

      assertFalse(request.resourceRequestSetCancelCallback { cancels.addAndFetch(1) })
      assertEquals(1, request.bindingCallbacks.rootCountForTesting())
      // Native keeps the first registration, and the rejected second one keeps no root.
      assertFailsWith<InvalidStateException> { request.resourceRequestSetCancelCallback {} }
      assertEquals(1, request.bindingCallbacks.rootCountForTesting())

      // Releasing the unanswered request fails it and releases the callback, which can no longer
      // run. A close that lands while the provider is still deciding releases the request once
      // the decision returns, so the case drains the request before it counts. The failure still
      // on its way to the map is not a child of the runtime, so the fixture's releases do not
      // wait for it.
      request.close()
      request.resourceRequestWaitUntilRetired()
      assertEquals(0, request.bindingCallbacks.rootCountForTesting())
      assertEquals(0, cancels.load())
    }
  }

  @Test
  fun aCancelCallbackRunsOnceWhenTheMapDropsItsRequest(): Unit = runSuspendTest {
    val claimed = CompletableDeferred<ResourceRequestHandle>()
    withMap(provider = claimingProvider(STYLE_URL, claimed)) {
      map.setStyleUrl(STYLE_URL).awaitWithin("the style command")
      val request = claimed.awaitWithin("the provider to claim the request")
      val cancelled = CompletableDeferred<Unit>()
      val cancels = AtomicInt(0)
      assertFalse(
        request.resourceRequestSetCancelCallback {
          cancels.addAndFetch(1)
          cancelled.complete(Unit)
        }
      )

      map.release().awaitWithin("the map release")
      cancelled.awaitWithin("the cancel callback")
      assertTrue(request.resourceRequestCancelled())
      // Release waits for a running cancel callback to return, and native releases the callback
      // once it has, so the root is gone once the request is.
      request.close()
      assertEquals(0, request.bindingCallbacks.rootCountForTesting())
      assertEquals(1, cancels.load())
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
    assertTrue(request.resourceRequestSetCancelCallback { cancels.addAndFetch(1) })
    assertEquals(0, request.bindingCallbacks.rootCountForTesting())
    assertEquals(0, cancels.load())
    request.close()
  }

  @Test
  fun aForbiddenCallInsideACallbackFailsAndAnAllowedOneSucceeds(): Unit = runSuspendTest {
    val outcome = CompletableDeferred<List<Throwable?>>()
    lateinit var fixtureRuntime: org.maplibre.nativeffi.generated.RuntimeHandle
    val provider = denyingProvider { request, handle ->
      if (request.requestedUrl != STYLE_URL) return@denyingProvider null
      val results =
        listOf(
          runCatching { fixtureRuntime.barrier() }.exceptionOrNull(),
          runCatching { fixtureRuntime.release() }.exceptionOrNull(),
          runCatching {
              handle.resourceRequestComplete(
                ResourceResponse(
                  ResourceResponseStatus.OK,
                  bytes = EMPTY_STYLE_JSON.encodeToByteArray(),
                )
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
      val (barrier, release, completion) = outcome.awaitWithin("the provider callback")
      assertTrue(barrier is InvalidStateException, "barrier inside the provider: $barrier")
      assertTrue(release is InvalidStateException, "release inside the provider: $release")
      assertEquals(null, completion, "completing the provided request is allowed")
      assertFalse(runtime.isClosed)
      awaitMapEvent(RuntimeEventType.MAP_STYLE_LOADED)
    }
  }

  @Test
  fun aProviderThatThrowsFailsItsRequestAndLeavesTheRuntimeWorking(): Unit = runSuspendTest {
    val provider = denyingProvider { request, _ ->
      if (request.requestedUrl == STYLE_URL) throw IllegalStateException("provider failure")
      null
    }
    withMap(provider = provider) {
      map.setStyleUrl(STYLE_URL).awaitWithin("the style command")
      // The binding contains the exception and answers native with a failed decision, which
      // MapLibre reports as the style's loading failure.
      awaitMapEvent(RuntimeEventType.MAP_LOADING_FAILED)
      loadStyle()
    }
  }

  @Test
  fun aClaimedRequestCanBeAnsweredAfterTheProviderReturns(): Unit = runSuspendTest {
    val claimed = CompletableDeferred<ResourceRequestHandle>()
    withMap(provider = claimingProvider(STYLE_URL, claimed)) {
      map.setStyleUrl(STYLE_URL).awaitWithin("the style command")
      val request = claimed.awaitWithin("the provider to claim the request")

      request.resourceRequestComplete(
        ResourceResponse(ResourceResponseStatus.OK, bytes = EMPTY_STYLE_JSON.encodeToByteArray())
      )
      // A request takes one response.
      assertFailsWith<InvalidStateException> {
        request.resourceRequestComplete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
      }
      request.close()
      awaitMapEvent(RuntimeEventType.MAP_STYLE_LOADED)
    }
  }

  @Test
  fun aTransformResponseRefusesUseAfterItsCallbackAndFromAnotherThread(): Unit = runSuspendTest {
    val outcome = CompletableDeferred<Pair<Throwable?, Throwable?>>()
    var escaped: ResourceTransformResponse? = null
    val transform = ResourceTransform { _, url, response ->
      if (url != STYLE_URL || outcome.isCompleted) return@ResourceTransform
      escaped = response
      var fromAnotherThread: Throwable? = null
      runOnBackgroundThread {
        fromAnotherThread =
          runCatching { GeneratedApi.resourceTransformResponseSetUrl(response, REWRITTEN_URL) }
            .exceptionOrNull()
      }
      val inside =
        runCatching { GeneratedApi.resourceTransformResponseSetUrl(response, REWRITTEN_URL) }
          .exceptionOrNull()
      outcome.complete(inside to fromAnotherThread)
    }
    // The provider passes the style through to the online source, which applies the transform.
    withMap(
      provider =
        denyingProvider { request, _ ->
          if (request.requestedUrl == STYLE_URL) ResourceProviderDecision.PASS_THROUGH else null
        }
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
  fun aResponseWithAnEmbeddedNulIsRejectedAndTheRequestStaysAnswerable(): Unit = runSuspendTest {
    val claimed = CompletableDeferred<ResourceRequestHandle>()
    withMap(provider = claimingProvider(STYLE_URL, claimed)) {
      map.setStyleUrl(STYLE_URL).awaitWithin("the style command")
      val request = claimed.awaitWithin("the provider to claim the request")
      assertFailsWith<InvalidArgumentException> {
        request.resourceRequestComplete(
          ResourceResponse(ResourceResponseStatus.ERROR, errorMessage = "bad\u0000message")
        )
      }
      request.resourceRequestComplete(
        ResourceResponse(ResourceResponseStatus.OK, bytes = EMPTY_STYLE_JSON.encodeToByteArray())
      )
      request.close()
      awaitMapEvent(RuntimeEventType.MAP_STYLE_LOADED)
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
  }
}
