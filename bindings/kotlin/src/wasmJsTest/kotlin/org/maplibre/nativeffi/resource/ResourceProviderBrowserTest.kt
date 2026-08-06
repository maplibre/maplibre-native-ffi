package org.maplibre.nativeffi.resource

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.drain
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.callback.QueuedResourceProviders
import org.maplibre.nativeffi.internal.callback.ResourceRewriteRules
import org.maplibre.nativeffi.internal.wasm.InjectedFaults
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.pageOrigin
import org.maplibre.nativeffi.pumpTurns
import org.maplibre.nativeffi.pumpUntil
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.waitForMapEvent
import org.maplibre.nativeffi.withMap

/**
 * The resource provider, which reaches host code through the module's record ring.
 *
 * MapLibre raises a provider callback on whichever thread wants the resource, and none of those may
 * enter this WebAssembly instance. So this binding registers `mln_adapter_queued_resource_provider`
 * rather than a callback of its own: the routes it claims are declared at registration, native
 * decides ownership by matching them, and a claimed request is copied into the ring and handed to
 * host code on the next pump.
 *
 * What follows is the shape of every test here. A request the host is meant to see is one a route
 * claims; a request no route claims never arrives at all, and goes on through native loading. And
 * because the body runs after the pump's own C call has returned, host code inside it is on an
 * ordinary stack and may call the map and the runtime freely.
 */
class ResourceProviderBrowserTest {
  // Spec coverage: BND-121, BND-122, BND-140, BND-142, BND-143, BND-144, BND-146, BND-147,
  // BND-148, BND-149, BND-151, BND-152, BND-154, BND-155, BND-156, BND-157.

  @Test
  fun aClaimedRequestCompletedInsideTheCallbackLoadsTheStyle() {
    withMap { runtime, map ->
      var calls = 0
      var copiedRequest: ResourceRequest? = null
      var callbackFailure: Throwable? = null

      runtime.setResourceProvider(listOf(route(STYLE_URL))) { request, handle ->
        try {
          calls++
          copiedRequest = request
          assertEquals(ResourceKind.STYLE, request.kind)
          // The pump's own C call has already returned by the time this runs, so this is an
          // ordinary stack: reaching the map from here is a same-thread call like any other. The
          // style it is waiting for is the one this callback has still to answer, so it reads as
          // not yet loaded.
          assertFalse(map.isFullyLoaded)
          handle.complete(
            ResourceResponse(ResourceResponseStatus.OK).apply {
              bytes = EMPTY_STYLE_JSON.encodeToByteArray()
            }
          )
          // Completing consumed the request's one answer.
          assertFailsWith<InvalidStateException> {
            handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
          }
          assertFailsWith<InvalidStateException> { handle.isCancelled() }
        } catch (failure: Throwable) {
          callbackFailure = failure
        }
      }

      map.setStyleUrl(STYLE_URL)
      waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED)

      assertNull(callbackFailure)
      assertEquals(1, calls)
      // The request the callback was handed is a copied value, so it still reads after the native
      // record it was decoded from has been released.
      val copied = assertNotNull(copiedRequest)
      assertEquals(STYLE_URL, copied.requestedUrl)
      assertEquals(ResourceKind.STYLE, copied.kind)
      assertEquals(0, copied.priorData.size)
    }
  }

  @Test
  fun aClaimedRequestCompletedAfterTheCallbackReturnsLoadsTheStyle() {
    withMap { runtime, map ->
      var handled: ResourceRequestHandle? = null
      runtime.setResourceProvider(listOf(route(STYLE_URL))) { _, handle -> handled = handle }

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

  /**
   * A glob route claims the URLs its pattern matches, and only those.
   *
   * This is where a queued provider parts company with a callback one. A callback binding decides
   * per request and can always fall back to pass-through; here the decision is native's, made
   * against a table declared before any request existed. So both halves have to be shown: a URL the
   * pattern matches arrives, and one it does not never reaches host code at all — it goes on
   * through native loading, which fails because nothing serves this scheme.
   *
   * The unmatched URL is one a careless pattern would claim. A `*` stops at a path separator, which
   * is what keeps a route for one directory from claiming everything below it, so a URL that
   * differs only by a further segment is the case worth spending a load on.
   */
  // Spec coverage: BND-142, BND-156.
  @Test
  fun aGlobRouteClaimsWhatItMatchesAndLeavesTheRestToNativeLoading() {
    withMap { runtime, map ->
      val claimed = mutableListOf<String>()
      runtime.setResourceProvider(listOf(route(HANDLED_PREFIX + "*", matchGlob = true))) {
        request,
        handle ->
        claimed += request.requestedUrl
        handle.complete(
          ResourceResponse(ResourceResponseStatus.OK).apply {
            bytes = EMPTY_STYLE_JSON.encodeToByteArray()
          }
        )
      }

      map.setStyleUrl(HANDLED_PREFIX + "style.json")
      waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED)
      assertEquals(listOf(HANDLED_PREFIX + "style.json"), claimed)

      // One segment deeper, so the pattern leaves it alone and native loading reports the failure.
      loadUnservedStyle(runtime, map, HANDLED_PREFIX + "deep/style.json")
      assertEquals(1, claimed.size, "a URL past the pattern's segment reached the provider")

      // And a URL that shares nothing with the pattern.
      loadUnservedStyle(runtime, map, UNCLAIMED_URL)
      assertEquals(1, claimed.size, "an unmatched request reached the provider anyway")
    }
  }

  /**
   * Which of a request's two URLs a route compares.
   *
   * A configured URI-scheme alias makes them differ: the requested URL keeps the alias the host
   * asked for, and the resolved URL is what the tile server normalizes it to. A route names one or
   * the other, and both have to claim the same request.
   */
  // Spec coverage: BND-155, BND-157.
  @Test
  fun aRouteClaimsAnAliasedRequestByEitherOfItsUrls() {
    val byRequested = claimAlias(route(ALIAS_URL, useRequestedUrl = true))
    assertEquals(ALIAS_URL, byRequested.requestedUrl)
    assertEquals(RESOLVED_ALIAS_URL, byRequested.resolvedUrl)

    // The same request, claimed by the URL the tile server normalized it to. A route comparing the
    // resolved URL would never match the alias, and one comparing the requested URL would never
    // match this, so the two together say the binding hands each flag to the right field.
    val byResolved = claimAlias(route(RESOLVED_ALIAS_URL))
    assertEquals(ALIAS_URL, byResolved.requestedUrl)
    assertEquals(RESOLVED_ALIAS_URL, byResolved.resolvedUrl)
  }

  /**
   * Loads the aliased style through a provider claiming it with [route], and reports the request.
   */
  private fun claimAlias(route: ResourceProviderRoute): ResourceRequest = withMap { runtime, map ->
    var claimed: ResourceRequest? = null
    runtime.setResourceProvider(listOf(route)) { request, handle ->
      claimed = request
      handle.complete(
        ResourceResponse(ResourceResponseStatus.OK).apply {
          bytes = EMPTY_STYLE_JSON.encodeToByteArray()
        }
      )
    }
    map.setStyleUrl(ALIAS_URL)
    waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED)
    assertNotNull(claimed, "the route never claimed the aliased request")
  }

  // Spec coverage: BND-147, BND-151.
  @Test
  fun aReleasedHandleAnswersNothingAndCannotReachALaterRequest() {
    withMap { runtime, map ->
      val handles = mutableListOf<ResourceRequestHandle>()
      runtime.setResourceProvider(listOf(route(HANDLED_PREFIX + "*", matchGlob = true))) { _, handle
        ->
        handles += handle
      }

      map.setStyleUrl(HANDLED_PREFIX + "first.json")
      pumpUntil(runtime) { handles.size == 1 }
      val first = handles.first()

      // Released without an answer: every later operation reports the handle as closed rather than
      // reaching whatever native request now occupies that storage.
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

  // Spec coverage: BND-146, BND-148, BND-152.
  @Test
  fun cancellationIsVisibleBeforeALateCompletionIsRefused() {
    withMap { runtime, map ->
      var handled: ResourceRequestHandle? = null
      runtime.setResourceProvider(listOf(route(STYLE_URL))) { _, handle -> handled = handle }

      map.setStyleUrl(STYLE_URL)
      pumpUntil(runtime) { handled != null }
      val handle = assertNotNull(handled)

      // Loading another style abandons the request in flight, which is what native cancels.
      map.setStyleJson(EMPTY_STYLE_JSON)
      assertTrue(
        pumpUntil(runtime) { handle.isCancelled() },
        "the abandoned request was never reported as cancelled",
      )

      // A completion that arrives after cancellation still reaches native, and native's refusal is
      // what the caller sees.
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

  // Spec coverage: BND-149.
  @Test
  fun anErrorResponseBecomesACopiedLoadingFailureEvent() {
    withMap { runtime, map ->
      runtime.setResourceProvider(listOf(route(STYLE_URL))) { _, handle ->
        handle.complete(
          ResourceResponse(ResourceResponseStatus.ERROR).apply {
            errorReason = ResourceErrorReason.NOT_FOUND
            errorMessage = "custom style failed"
          }
        )
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

  // Spec coverage: BND-154.
  @Test
  fun aProviderIsConsultedUntilItIsReplacedAndThenUntilItIsCleared() {
    withMap { runtime, map ->
      var first = 0
      var second = 0

      runtime.setResourceProvider(listOf(route(HANDLED_PREFIX + "*", matchGlob = true))) { _, handle
        ->
        first++
        handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
      }
      loadHandledStyle(runtime, map, HANDLED_PREFIX + "first.json")
      assertTrue(first > 0)

      // Replacing while a map is live is part of the C API's contract, and the routes go with the
      // callback: the replacement claims a prefix of its own.
      runtime.setResourceProvider(listOf(route(REPLACEMENT_PREFIX + "*", matchGlob = true))) {
        _,
        handle ->
        second++
        handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
      }
      val firstAfterReplace = first
      loadHandledStyle(runtime, map, REPLACEMENT_PREFIX + "second.json")
      assertTrue(second > 0)
      assertEquals(firstAfterReplace, first, "the replaced provider was consulted again")

      // The replacement took the previous routes away with it, so the prefix the first one claimed
      // is nobody's now and passes through to native loading.
      loadUnservedStyle(runtime, map, HANDLED_PREFIX + "third.json")
      assertEquals(firstAfterReplace, first)

      runtime.clearResourceProvider()
      val secondAfterClear = second
      loadUnservedStyle(runtime, map, REPLACEMENT_PREFIX + "fourth.json")
      assertEquals(secondAfterClear, second, "a cleared provider was consulted again")

      // Clearing an already cleared provider stays a successful no-op.
      runtime.clearResourceProvider()
    }
  }

  /**
   * A replacement native refuses leaves the provider that was already there serving requests.
   *
   * The order the binding installs in is what this rests on. The replacement's routes and listener
   * state go into the module's heap before native is told about them, because the provider struct
   * native is given points at them. So at the moment native refuses, the binding holds state for a
   * provider native has never heard of; it has to release that and keep the previous one.
   *
   * Native has no refusal of its own to offer here — setting a provider validates the runtime and
   * the descriptor, both of which the binding has already made valid — so the refusal is injected.
   */
  // Spec coverage: BND-122.
  @Test
  fun aProviderReplacementNativeRefusesKeepsThePreviousProvider() {
    withMap { runtime, map ->
      var previous = 0
      var replacement = 0
      runtime.setResourceProvider(listOf(route(HANDLED_PREFIX + "*", matchGlob = true))) { _, handle
        ->
        previous++
        handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
      }
      loadHandledStyle(runtime, map, HANDLED_PREFIX + "installed.json")
      assertTrue(previous > 0, "the provider that was installed was never consulted")

      val registered = QueuedResourceProviders.liveRegistrations
      try {
        InjectedFaults.failNextCall(
          "mln_runtime_set_resource_provider",
          MaplibreStatus.INVALID_ARGUMENT,
          "provider callback must not be null",
        )
        val error =
          assertFailsWith<InvalidArgumentException> {
            runtime.setResourceProvider(
              listOf(route(REPLACEMENT_PREFIX + "*", matchGlob = true))
            ) { _, handle ->
              replacement++
              handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
            }
          }
        assertEquals(MaplibreStatus.INVALID_ARGUMENT, error.status)
        assertEquals("provider callback must not be null", error.diagnostic)
      } finally {
        InjectedFaults.reset()
      }

      // The replacement's state went back, so the module is not holding routes for a provider
      // native was never given.
      assertEquals(
        registered,
        QueuedResourceProviders.liveRegistrations,
        "the refusal did not leave exactly the previous registration standing",
      )

      // And native still reaches the provider it already had, which is the half the count cannot
      // show: state that stayed and state that is still wired to native look the same.
      val beforeLoad = previous
      loadHandledStyle(runtime, map, HANDLED_PREFIX + "refused.json")
      assertTrue(previous > beforeLoad, "the previous provider stopped being consulted")
      assertEquals(0, replacement, "the provider native refused was consulted anyway")

      // A later replacement is accepted, so the refusal left the runtime able to take one.
      runtime.setResourceProvider(listOf(route(REPLACEMENT_PREFIX + "*", matchGlob = true))) {
        _,
        handle ->
        replacement++
        handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
      }
      loadHandledStyle(runtime, map, REPLACEMENT_PREFIX + "accepted.json")
      assertTrue(replacement > 0)
      runtime.clearResourceProvider()
    }
  }

  /**
   * URL rewriting, which this binding does with a native rule table rather than a callback.
   *
   * MapLibre consults a resource transform on the thread that is about to fetch, so there is no
   * host callback to consult and nothing to observe on the way through. What a rule changes is
   * where the request goes, so that is what is asserted: the same style URL fails one way with the
   * rules installed and another way without them. A 404 says the request went to the URL the map
   * was given; anything else says it went somewhere else.
   */
  // Spec coverage: BND-140.
  @Test
  fun aRewriteRuleSendsARequestElsewhereUntilTheRulesAreCleared() {
    withMap { runtime, map ->
      val source = pageOrigin() + "/mln-test/rewrite-source.json"

      runtime.setResourceUrlRewriteRules(
        listOf(ResourceUrlRewriteRule(url = source, replacementUrl = REWRITE_TARGET_URL))
      )
      val rewritten = failedLoadMessage(runtime, map, source)
      assertFalse(
        rewritten.contains("404"),
        "the request was not rewritten: native loading reported $rewritten",
      )

      // Cleared, so the same URL is fetched unchanged and the origin answers for it.
      runtime.clearResourceTransform()
      val direct = failedLoadMessage(runtime, map, source)
      assertTrue(
        direct.contains("404"),
        "a cleared rule table still rewrote the request: native loading reported $direct",
      )

      // Clearing one that is already cleared stays a successful no-op.
      runtime.clearResourceTransform()
    }
  }

  /** The rule table is the other family installed this way, and it is refused the same way. */
  // Spec coverage: BND-122.
  @Test
  fun aRewriteRuleReplacementNativeRefusesKeepsThePreviousRules() {
    withMap { runtime, map ->
      val source = pageOrigin() + "/mln-test/refused-rewrite.json"
      runtime.setResourceUrlRewriteRules(
        listOf(ResourceUrlRewriteRule(url = source, replacementUrl = REWRITE_TARGET_URL))
      )

      val registered = ResourceRewriteRules.liveRegistrations
      try {
        InjectedFaults.failNextCall(
          "mln_runtime_set_resource_transform",
          MaplibreStatus.INVALID_ARGUMENT,
          "transform callback must not be null",
        )
        assertFailsWith<InvalidArgumentException> {
          runtime.setResourceUrlRewriteRules(
            listOf(ResourceUrlRewriteRule(url = source, replacementUrl = null))
          )
        }
      } finally {
        InjectedFaults.reset()
      }

      assertEquals(
        registered,
        ResourceRewriteRules.liveRegistrations,
        "the refusal did not leave exactly the previous rule table standing",
      )

      // The rules native already had are the ones still in force.
      val message = failedLoadMessage(runtime, map, source)
      assertFalse(message.contains("404"), "the previous rules stopped rewriting: $message")
      runtime.clearResourceTransform()
    }
  }

  // Spec coverage: BND-121.
  @Test
  fun aFailingCallbackDoesNotEscapeIntoNativeAndTheRuntimeKeepsWorking() {
    withMap { runtime, map ->
      var calls = 0
      val stranded = mutableListOf<ResourceRequestHandle>()
      runtime.setResourceProvider(listOf(route(HANDLED_PREFIX + "*", matchGlob = true))) { _, handle
        ->
        calls++
        stranded += handle
        throw IllegalStateException("contained")
      }

      // Nothing unwinds through the drain, and the drain goes on running. The request the body
      // never answered stays the host's, which is why it is released below.
      map.setStyleUrl(HANDLED_PREFIX + "failing.json")
      assertTrue(pumpUntil(runtime) { calls > 0 }, "the provider was never consulted")
      stranded.forEach { it.close() }

      // The runtime is unharmed, and a provider that answers still works afterwards.
      runtime.setResourceProvider(listOf(route(STYLE_URL))) { _, handle ->
        handle.complete(
          ResourceResponse(ResourceResponseStatus.OK).apply {
            bytes = EMPTY_STYLE_JSON.encodeToByteArray()
          }
        )
      }
      map.setStyleUrl(STYLE_URL)
      waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED)
    }
  }

  /**
   * Replacing or clearing the provider from inside a delivered callback.
   *
   * The body runs inside the drain that delivered it, so retiring its registration would be a close
   * waiting on the frame below it. There is one thread and one stack here, so that wait can never
   * finish and is refused instead.
   */
  @Test
  fun aProviderCannotBeReplacedOrClearedFromInsideItsOwnCallback() {
    withMap { runtime, map ->
      var replaceError: Throwable? = null
      var clearError: Throwable? = null

      runtime.setResourceProvider(listOf(route(HANDLED_PREFIX + "*", matchGlob = true))) { _, handle
        ->
        replaceError =
          runCatching { runtime.setResourceProvider(listOf(route(STYLE_URL))) { _, _ -> } }
            .exceptionOrNull()
        clearError = runCatching { runtime.clearResourceProvider() }.exceptionOrNull()
        handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
      }

      map.setStyleUrl(HANDLED_PREFIX + "reentrant.json")
      assertTrue(pumpUntil(runtime) { replaceError != null }, "the provider was never consulted")

      assertTrue(replaceError is InvalidStateException, "replace reported $replaceError")
      assertTrue(clearError is InvalidStateException, "clear reported $clearError")
    }
  }

  private fun route(
    url: String,
    matchGlob: Boolean = false,
    useRequestedUrl: Boolean = false,
  ): ResourceProviderRoute =
    ResourceProviderRoute(url = url, matchGlob = matchGlob, useRequestedUrl = useRequestedUrl)

  /** Loads a style the provider answers with no content, so the load fails after it was claimed. */
  private fun loadHandledStyle(runtime: RuntimeHandle, map: MapHandle, styleUrl: String) {
    drain(runtime)
    map.setStyleUrl(styleUrl)
    waitForMapEvent(runtime, map, RuntimeEventType.MAP_LOADING_FAILED)
  }

  /** Loads a style whose scheme no file source serves, so native loading reports the failure. */
  private fun loadUnservedStyle(runtime: RuntimeHandle, map: MapHandle, styleUrl: String) {
    drain(runtime)
    map.setStyleUrl(styleUrl)
    waitForMapEvent(runtime, map, RuntimeEventType.MAP_LOADING_FAILED)
    // Long enough that a provider still claiming this prefix would have been consulted.
    pumpTurns(runtime, QUIET_PUMPS)
  }

  /** Loads [styleUrl] and reports the message of the failure it produces. */
  private fun failedLoadMessage(runtime: RuntimeHandle, map: MapHandle, styleUrl: String): String {
    drain(runtime)
    map.setStyleUrl(styleUrl)
    return waitForMapEvent(runtime, map, RuntimeEventType.MAP_LOADING_FAILED).message
  }

  private companion object {
    /** A scheme no file source serves, so a request for it reaches native loading and fails. */
    const val UNCLAIMED_URL = "jar:file:/packaged/style.json"
    const val STYLE_URL = "custom://style.json"
    const val HANDLED_PREFIX = "custom://handled/"
    const val REPLACEMENT_PREFIX = "custom://replacement/"

    /** The default tile server's alias, and what it normalizes to. */
    const val ALIAS_URL = "maplibre://maps/style"
    const val RESOLVED_ALIAS_URL = "https://demotiles.maplibre.org/style.json"

    /**
     * A host no name server resolves.
     *
     * A rewritten request reaches the network and fails to connect, which is a different failure
     * from the 404 the unrewritten URL gets — and it is the difference that says the rule fired.
     */
    const val REWRITE_TARGET_URL = "https://rewritten.invalid/style.json"

    /** Long enough that a provider still installed would have been consulted at least once. */
    const val QUIET_PUMPS = 200
  }
}
