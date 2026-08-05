package org.maplibre.nativeffi.resource

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.browserTest
import org.maplibre.nativeffi.drain
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.wasm.InjectedFaults
import org.maplibre.nativeffi.internal.wasm.ResourceProviderBridge
import org.maplibre.nativeffi.internal.wasm.ResourceTransformBridge
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.maplibreScope
import org.maplibre.nativeffi.pageOrigin
import org.maplibre.nativeffi.pumpUntil
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.waitForMapEvent
import org.maplibre.nativeffi.withMap

/**
 * The resource provider, which is the one callback that travels from native into the page.
 *
 * A MapLibre worker cannot enter this module's WebAssembly instance, so the module's own thunk
 * forwards the request to the page and blocks the worker until the page answers. The page is not
 * blocked — it is inside a parked pump — so the turn that delivers the request always comes. That
 * makes a provider callback one of the two places in this binding where host code may not reach the
 * owner thread, which is why a request handle's own operations are the only ones that do not
 * dispatch. What decides that is the blocked worker rather than the stack: a custom geometry
 * source's tile callback is delivered on a stack that may park, because nothing is waiting on it.
 */
class ResourceProviderBrowserTest {
  // Spec coverage: BND-121, BND-122, BND-141, BND-142, BND-143, BND-144, BND-146, BND-147,
  // BND-148, BND-140, BND-149, BND-150, BND-151, BND-152, BND-154, BND-155.

  @Test
  fun aHandledRequestCompletedInsideTheCallbackLoadsTheStyle(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap { runtime, map ->
        var calls = 0
        var copiedRequest: ResourceRequest? = null
        var callbackFailure: Throwable? = null

        runtime.setResourceProvider { request, handle ->
          if (request.requestedUrl != STYLE_URL) {
            return@setResourceProvider ResourceProviderDecision.PASS_THROUGH
          }
          try {
            calls++
            copiedRequest = request
            assertEquals(ResourceKind.STYLE, request.kind)
            // The one thing this stack may not do, and the reason is the blocked worker rather
            // than the stack: a call to the owner thread would park the page while a MapLibre
            // worker is parked on the page, which is the cycle that would take both sides down.
            // The refusal names the callback instead. A custom geometry source's tile callback is
            // in the other position — nothing waits for it — and may call the map freely.
            val refused = assertFailsWith<InvalidStateException> { map.isFullyLoaded }
            assertTrue(
              refused.message.orEmpty().contains("callback"),
              "the refusal does not say a callback is what refused it: ${refused.message}",
            )
            handle.complete(
              ResourceResponse(ResourceResponseStatus.OK).apply {
                bytes = EMPTY_STYLE_JSON.encodeToByteArray()
              }
            )
            // Completing consumed the request's one answer, so the handle is finished even though
            // the callback goes on to return pass-through below.
            assertFailsWith<InvalidStateException> {
              handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
            }
            assertFailsWith<InvalidStateException> { handle.isCancelled() }
          } catch (failure: Throwable) {
            callbackFailure = failure
          }
          // Pass-through after an inline completion: the request is already answered, so the
          // decision cannot hand it back to native loading.
          ResourceProviderDecision.PASS_THROUGH
        }

        map.setStyleUrl(STYLE_URL)
        waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED)

        assertNull(callbackFailure)
        assertEquals(1, calls)
        // The request the callback was handed is a copied value, so it still reads after the
        // native request it described has gone.
        val copied = assertNotNull(copiedRequest)
        assertEquals(STYLE_URL, copied.requestedUrl)
        assertEquals(ResourceKind.STYLE, copied.kind)
        assertEquals(0, copied.priorData.size)
      }
    }
  }

  @Test
  fun aHandledRequestCompletedAfterTheCallbackReturnsLoadsTheStyle(): Promise<JsAny?> =
    browserTest {
      maplibreScope {
        withMap { runtime, map ->
          var handled: ResourceRequestHandle? = null
          runtime.setResourceProvider { request, handle ->
            if (request.requestedUrl != STYLE_URL) {
              return@setResourceProvider ResourceProviderDecision.PASS_THROUGH
            }
            handled = handle
            ResourceProviderDecision.HANDLE
          }

          map.setStyleUrl(STYLE_URL)
          pumpUntil(runtime) { handled != null }
          val handle = assertNotNull(handled, "the provider was never asked for the style")

          // Outstanding, and native has not cancelled it.
          assertFalse(handle.isCancelled())
          handle.complete(
            ResourceResponse(ResourceResponseStatus.OK).apply {
              bytes = EMPTY_STYLE_JSON.encodeToByteArray()
            }
          )

          // Completion is terminal: a second one is the binding's already-completed error, raised
          // before anything crosses into the module.
          val second =
            assertFailsWith<InvalidStateException> {
              handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
            }
          assertEquals(MaplibreStatus.INVALID_STATE, second.status)
          assertFailsWith<InvalidStateException> { handle.isCancelled() }

          handle.close()
          waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED)
        }
      }
    }

  @Test
  fun aPassedThroughRequestKeepsNoHandleTheHostCanStillUse(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap { runtime, map ->
        var passedThrough: ResourceRequestHandle? = null
        runtime.setResourceProvider { request, handle ->
          if (request.requestedUrl == UNSERVED_URL) passedThrough = handle
          ResourceProviderDecision.PASS_THROUGH
        }

        map.setStyleUrl(UNSERVED_URL)
        val failure = waitForMapEvent(runtime, map, RuntimeEventType.MAP_LOADING_FAILED)
        assertEquals(map, failure.mapSource)

        // The decision gave the request back to native loading, so the handle the callback held is
        // no longer the host's to answer with.
        val handle = assertNotNull(passedThrough)
        assertFailsWith<InvalidStateException> { handle.isCancelled() }
        assertFailsWith<InvalidStateException> {
          handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
        }
      }
    }
  }

  @Test
  fun aReleasedHandleAnswersNothingAndCannotReachALaterRequest(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap { runtime, map ->
        val handles = mutableListOf<ResourceRequestHandle>()
        runtime.setResourceProvider { request, handle ->
          if (request.requestedUrl.startsWith(HANDLED_PREFIX)) {
            handles += handle
            return@setResourceProvider ResourceProviderDecision.HANDLE
          }
          ResourceProviderDecision.PASS_THROUGH
        }

        map.setStyleUrl(HANDLED_PREFIX + "first.json")
        pumpUntil(runtime) { handles.size == 1 }
        val first = handles.first()

        // Released without an answer: every later operation reports the handle as closed rather
        // than reaching whatever native request now occupies that storage.
        first.close()
        first.close()
        assertFailsWith<InvalidStateException> { first.isCancelled() }
        assertFailsWith<InvalidStateException> {
          first.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
        }

        // A second request comes in and is answered normally, so the stale handle above could not
        // have interfered with it.
        map.setStyleUrl(HANDLED_PREFIX + "second.json")
        pumpUntil(runtime) { handles.size == 2 }
        val second = handles[1]
        assertFailsWith<InvalidStateException> { first.isCancelled() }
        second.complete(
          ResourceResponse(ResourceResponseStatus.OK).apply {
            bytes = EMPTY_STYLE_JSON.encodeToByteArray()
          }
        )
        second.close()
        waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED)
      }
    }
  }

  @Test
  fun cancellationIsVisibleBeforeALateCompletionIsRefused(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap { runtime, map ->
        var handled: ResourceRequestHandle? = null
        runtime.setResourceProvider { request, handle ->
          if (request.requestedUrl != STYLE_URL) {
            return@setResourceProvider ResourceProviderDecision.PASS_THROUGH
          }
          handled = handle
          ResourceProviderDecision.HANDLE
        }

        map.setStyleUrl(STYLE_URL)
        pumpUntil(runtime) { handled != null }
        val handle = assertNotNull(handled)

        // Loading another style abandons the request in flight, which is what native cancels.
        map.setStyleJson(EMPTY_STYLE_JSON)
        assertTrue(
          pumpUntil(runtime) { handle.isCancelled() },
          "the abandoned request was never reported as cancelled",
        )

        // A completion that arrives after cancellation still reaches native, and native's refusal
        // is what the caller sees.
        val late =
          assertFailsWith<InvalidStateException> {
            handle.complete(
              ResourceResponse(ResourceResponseStatus.OK).apply {
                bytes = EMPTY_STYLE_JSON.encodeToByteArray()
              }
            )
          }
        assertEquals(MaplibreStatus.INVALID_STATE, late.status)

        // And that completion was terminal even though native refused it, so the handle is spent.
        assertFailsWith<InvalidStateException> { handle.isCancelled() }
        handle.close()
      }
    }
  }

  @Test
  fun anErrorResponseBecomesACopiedLoadingFailureEvent(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap { runtime, map ->
        runtime.setResourceProvider { request, handle ->
          if (request.requestedUrl != STYLE_URL) {
            return@setResourceProvider ResourceProviderDecision.PASS_THROUGH
          }
          handle.complete(
            ResourceResponse(ResourceResponseStatus.ERROR).apply {
              errorReason = ResourceErrorReason.NOT_FOUND
              errorMessage = "custom style failed"
            }
          )
          ResourceProviderDecision.HANDLE
        }

        map.setStyleUrl(STYLE_URL)
        val failure = waitForMapEvent(runtime, map, RuntimeEventType.MAP_LOADING_FAILED)
        val copiedMessage = failure.message

        assertEquals(map, failure.mapSource)
        assertTrue(copiedMessage.contains("custom style failed"), copiedMessage)
        // The message came out of storage the next poll reuses.
        runtime.pollEvent()
        assertEquals(copiedMessage, failure.message)
      }
    }
  }

  @Test
  fun aProviderIsConsultedUntilItIsReplacedAndThenUntilItIsCleared(): Promise<JsAny?> =
    browserTest {
      maplibreScope {
        withMap { runtime, map ->
          var first = 0
          var second = 0

          runtime.setResourceProvider { _, _ ->
            first++
            ResourceProviderDecision.PASS_THROUGH
          }
          loadUnservedStyle(runtime, map, UNSERVED_URL + "?first")
          assertTrue(first > 0)

          // Replacing while a map is live is part of the C API's contract.
          runtime.setResourceProvider { _, _ ->
            second++
            ResourceProviderDecision.PASS_THROUGH
          }
          val firstAfterReplace = first
          loadUnservedStyle(runtime, map, UNSERVED_URL + "?second")
          assertTrue(second > 0)
          assertEquals(firstAfterReplace, first)

          runtime.clearResourceProvider()
          val secondAfterClear = second
          loadUnservedStyle(runtime, map, UNSERVED_URL + "?third")
          assertEquals(firstAfterReplace, first)
          assertEquals(secondAfterClear, second)

          // Clearing an already cleared provider stays a successful no-op.
          runtime.clearResourceProvider()
        }
      }
    }

  /**
   * A replacement native refuses leaves the provider that was already there serving requests.
   *
   * The order the binding installs in is what this rests on. The replacement's host trampoline goes
   * in before native is told about it, because the module's thunk reaches the page through a
   * pointer it reads on every request — so at the moment native refuses, the binding is holding a
   * registration for a provider native has never heard of. It has to give that one back and keep
   * the previous one, and the two halves are separately observable: the registry count says the
   * replacement's state went, and a real request says whose callback native still reaches.
   *
   * Native has no refusal of its own to offer here — setting a provider validates the runtime and
   * the descriptor, both of which the binding has already made valid — so the refusal is injected.
   */
  // Spec coverage: BND-122.
  @Test
  fun aProviderReplacementNativeRefusesKeepsThePreviousProvider(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap { runtime, map ->
        var previous = 0
        var replacement = 0
        runtime.setResourceProvider { _, _ ->
          previous++
          ResourceProviderDecision.PASS_THROUGH
        }
        loadUnservedStyle(runtime, map, UNSERVED_URL + "?installed")
        assertTrue(previous > 0, "the provider that was installed was never consulted")

        val registered = ResourceProviderBridge.liveRegistrations
        try {
          InjectedFaults.failNextCall(
            "mln_runtime_set_resource_provider",
            MaplibreStatus.INVALID_ARGUMENT,
            "provider callback must not be null",
          )
          val error =
            assertFailsWith<InvalidArgumentException> {
              runtime.setResourceProvider { _, _ ->
                replacement++
                ResourceProviderDecision.PASS_THROUGH
              }
            }
          assertEquals(MaplibreStatus.INVALID_ARGUMENT, error.status)
          assertEquals("provider callback must not be null", error.diagnostic)
        } finally {
          InjectedFaults.reset()
        }

        // The replacement's registration went back, so the module's trampoline is not held open for
        // a provider native was never given.
        assertEquals(
          registered,
          ResourceProviderBridge.liveRegistrations,
          "the refusal did not leave exactly the previous registration standing",
        )

        // And native still reaches the provider it already had, which is the half the count cannot
        // show: a registration that stayed and one that is still wired to native look the same.
        val beforeLoad = previous
        loadUnservedStyle(runtime, map, UNSERVED_URL + "?refused")
        assertTrue(previous > beforeLoad, "the previous provider stopped being consulted")
        assertEquals(0, replacement, "the provider native refused was consulted anyway")

        // A later replacement is accepted, so the refusal left the runtime able to take one.
        runtime.setResourceProvider { _, _ ->
          replacement++
          ResourceProviderDecision.PASS_THROUGH
        }
        loadUnservedStyle(runtime, map, UNSERVED_URL + "?accepted")
        assertTrue(replacement > 0)
        runtime.clearResourceProvider()
      }
    }
  }

  /** The URL transform is the other family installed this way, and it is refused the same way. */
  // Spec coverage: BND-122.
  @Test
  fun aTransformReplacementNativeRefusesKeepsThePreviousTransform(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap { runtime, map ->
        var previous = 0
        var replacement = 0
        runtime.setResourceTransform { _ ->
          previous++
          null
        }
        // A real HTTP request, which is where MapLibre consults a transform; the URL is not served
        // and does not need to be, because what is asserted is the consultation.
        map.setStyleUrl(pageOrigin() + "/maplibre/transform-installed.json")
        assertTrue(
          pumpUntil(runtime) { previous > 0 },
          "the transform that was installed was never consulted",
        )

        val registered = ResourceTransformBridge.liveRegistrations
        try {
          InjectedFaults.failNextCall(
            "mln_runtime_set_resource_transform",
            MaplibreStatus.INVALID_ARGUMENT,
            "transform callback must not be null",
          )
          assertFailsWith<InvalidArgumentException> {
            runtime.setResourceTransform { _ ->
              replacement++
              null
            }
          }
        } finally {
          InjectedFaults.reset()
        }

        assertEquals(
          registered,
          ResourceTransformBridge.liveRegistrations,
          "the refusal did not leave exactly the previous registration standing",
        )

        val consultedBefore = previous
        map.setStyleUrl(pageOrigin() + "/maplibre/transform-refused.json")
        assertTrue(
          pumpUntil(runtime) { previous > consultedBefore },
          "the previous transform stopped being consulted",
        )
        assertEquals(0, replacement, "the transform native refused was consulted anyway")
        runtime.clearResourceTransform()
      }
    }
  }

  @Test
  fun aSchemeAliasReachesTheProviderAsBothItsUrls(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap { runtime, map ->
        var resolved: String? = null
        runtime.setResourceProvider { request, handle ->
          if (request.requestedUrl != ALIAS_URL) {
            return@setResourceProvider ResourceProviderDecision.PASS_THROUGH
          }
          resolved = request.resolvedUrl
          handle.complete(
            ResourceResponse(ResourceResponseStatus.OK).apply {
              bytes = EMPTY_STYLE_JSON.encodeToByteArray()
            }
          )
          ResourceProviderDecision.HANDLE
        }

        map.setStyleUrl(ALIAS_URL)
        waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED)

        // The requested URL keeps the alias the host asked for, and the resolved one is what the
        // configured tile server normalizes it to.
        assertEquals("https://demotiles.maplibre.org/style.json", resolved)
      }
    }
  }

  @Test
  fun aTransformRewritesARequestedUrlAndStopsOnceItIsCleared(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap { runtime, map ->
        val seen = mutableListOf<ResourceTransformRequest>()
        val original = pageOrigin() + "/maplibre/original-style.json"
        val rewritten = pageOrigin() + "/maplibre/rewritten-style.json"

        runtime.setResourceTransform { request ->
          seen += request
          if (request.url == original) rewritten else null
        }

        // A real HTTP request, so the transform sits where MapLibre actually consults it. Neither
        // URL is served, so what is asserted is the rewrite rather than the load.
        map.setStyleUrl(original)
        assertTrue(pumpUntil(runtime) { seen.isNotEmpty() }, "the transform was never consulted")

        val request = seen.first()
        assertEquals(original, request.url)
        assertEquals(ResourceKind.STYLE, request.kind)

        // Cleared after registration, and after a map has already used it: nothing reaches it
        // again.
        runtime.clearResourceTransform()
        val afterClear = seen.size
        map.setStyleUrl(pageOrigin() + "/maplibre/after-clear-style.json")
        repeat(QUIET_PUMPS) {
          runtime.pump(QUIET_PUMP_MILLIS)
          while (runtime.pollEvent() != null) {}
          assertEquals(afterClear, seen.size, "the cleared transform was consulted again")
        }

        // Clearing one that is already cleared stays a successful no-op.
        runtime.clearResourceTransform()
      }
    }
  }

  @Test
  fun aFailingCallbackDoesNotEscapeIntoNativeAndTheRuntimeKeepsWorking(): Promise<JsAny?> =
    browserTest {
      maplibreScope {
        withMap { runtime, map ->
          var calls = 0
          runtime.setResourceProvider { _, _ ->
            calls++
            throw IllegalStateException("contained")
          }

          // Nothing unwinds through the module: the failed callback is reported to native as a
          // provider error, and the map load fails rather than the page trapping.
          map.setStyleUrl(UNSERVED_URL)
          waitForMapEvent(runtime, map, RuntimeEventType.MAP_LOADING_FAILED)
          assertTrue(calls > 0)

          // The runtime is unharmed, and a provider that answers still works afterwards.
          runtime.setResourceProvider { request, handle ->
            if (request.requestedUrl != STYLE_URL) {
              return@setResourceProvider ResourceProviderDecision.PASS_THROUGH
            }
            handle.complete(
              ResourceResponse(ResourceResponseStatus.OK).apply {
                bytes = EMPTY_STYLE_JSON.encodeToByteArray()
              }
            )
            ResourceProviderDecision.HANDLE
          }
          map.setStyleUrl(STYLE_URL)
          waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED)
        }
      }
    }

  @Test
  fun aProviderCannotBeReplacedOrClearedFromInsideItsOwnCallback(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap { runtime, map ->
        var replaceError: Throwable? = null
        var clearError: Throwable? = null
        var dispatchError: Throwable? = null

        runtime.setResourceProvider { _, _ ->
          replaceError =
            runCatching {
                runtime.setResourceProvider { _, _ -> ResourceProviderDecision.PASS_THROUGH }
              }
              .exceptionOrNull()
          clearError = runCatching { runtime.clearResourceProvider() }.exceptionOrNull()
          // A callback runs on a stack that was never entered through the promising trampoline, so
          // an owner-thread call from it would trap rather than park. It is reported instead.
          dispatchError = runCatching { runtime.pump(0) }.exceptionOrNull()
          ResourceProviderDecision.PASS_THROUGH
        }

        map.setStyleUrl(UNSERVED_URL)
        waitForMapEvent(runtime, map, RuntimeEventType.MAP_LOADING_FAILED)

        assertTrue(replaceError is InvalidStateException, "replace reported $replaceError")
        assertTrue(clearError is InvalidStateException, "clear reported $clearError")
        assertTrue(dispatchError is InvalidStateException, "dispatch reported $dispatchError")
      }
    }
  }

  /**
   * Loads a style whose scheme no file source serves, so the request reaches the provider and the
   * loading failure that follows says it went on to native loading rather than being answered.
   */
  private fun loadUnservedStyle(runtime: RuntimeHandle, map: MapHandle, styleUrl: String) {
    // Drained first, so the failure waited for below is this load's rather than the last one's.
    drain(runtime)
    map.setStyleUrl(styleUrl)
    waitForMapEvent(runtime, map, RuntimeEventType.MAP_LOADING_FAILED)
  }

  private companion object {
    /** A scheme no file source serves, so every request for it reaches the provider and fails. */
    const val UNSERVED_URL = "jar:file:/packaged/style.json"
    const val STYLE_URL = "custom://style.json"
    const val HANDLED_PREFIX = "custom://handled/"
    /** The default tile server's alias, which resolves to a URL the provider also sees. */
    const val ALIAS_URL = "maplibre://maps/style"

    /** Long enough that a transform still installed would have been consulted at least once. */
    const val QUIET_PUMPS = 200
    const val QUIET_PUMP_MILLIS = 2L
  }
}
