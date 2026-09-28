package org.maplibre.nativeffi.runtime

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.generated.AmbientCacheOperation
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.NetworkStatus
import org.maplibre.nativeffi.generated.OfflineGeometryRegionDefinition
import org.maplibre.nativeffi.generated.OfflineRegionDefinition
import org.maplibre.nativeffi.generated.OfflineRegionDefinitionData
import org.maplibre.nativeffi.generated.OfflineRegionDownloadState
import org.maplibre.nativeffi.generated.ResourceErrorReason
import org.maplibre.nativeffi.generated.ResourceKind
import org.maplibre.nativeffi.generated.ResourceProvider
import org.maplibre.nativeffi.generated.ResourceProviderDecision
import org.maplibre.nativeffi.generated.ResourceRequestHandle
import org.maplibre.nativeffi.generated.ResourceResponse
import org.maplibre.nativeffi.generated.ResourceResponseStatus
import org.maplibre.nativeffi.generated.ResourceTransform
import org.maplibre.nativeffi.generated.RuntimeEvent
import org.maplibre.nativeffi.generated.RuntimeEventPayload
import org.maplibre.nativeffi.generated.RuntimeEventSourceType
import org.maplibre.nativeffi.generated.RuntimeEventType
import org.maplibre.nativeffi.generated.RuntimeHandle
import org.maplibre.nativeffi.sleepMillis

@OptIn(ExperimentalAtomicApi::class)
class RuntimeHandleTest {
  @Test
  fun runtimeRunsOnceAndCloses(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())

    assertFalse(runtime.isClosed)
    runtime.barrier().await()
    val tornDown = runtime.release()
    // A second close reports the same teardown instead of starting another one.
    assertEquals(tornDown, runtime.release())

    assertTrue(runtime.isClosed)
    assertFailsWith<InvalidStateException> { runtime.barrier().await() }
  }

  @Test
  fun runtimeCloseReportsTheEndOfNativeTeardown(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
            )
        )
        .await()
    map.setStyleUrl("custom://never-served.json").await()
    map.release()

    // The report arrives only after the released map's teardown finishes too.
    runtime.release().await()

    assertTrue(runtime.isClosed)
    assertTrue(map.isClosed)
  }

  @Test
  fun freshRuntimeDrainsAnEmptyBatch(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      assertEquals(emptyList(), runtime.drainEvents().use { it.get().events })
    }
  }

  @Test
  fun ambientCacheOperationRemainsUsableAfterRuntimeClose(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val completion = runtime.runAmbientCacheOperation(AmbientCacheOperation.INVALIDATE)
    runtime.release()
    assertTrue(runtime.isClosed)
    completion.await()
  }

  @Test
  fun setMaximumAmbientCacheSizeReachesNative(): Unit = runSuspendTest {
    val runtime =
      GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault().copy(cachePath = ":memory:"))
    runtime.setMaximumAmbientCacheSize(8uL shl 20).await()

    runtime.release()
  }

  @Test
  fun offlineDownloadStateUnknownRawValueIsRejectedByNative(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      assertFailsWith<InvalidArgumentException> {
        runtime.offlineRegionSetDownloadState(1, OfflineRegionDownloadState(900u)).await()
      }
    }
  }

  @Test
  fun geometryOfflineRegionDefinitionStartsOperation(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault().copy(cachePath = ":memory:"))
      .use { runtime ->
        val region =
          runtime
            .offlineRegionCreate(
              OfflineRegionDefinition(
                OfflineRegionDefinitionData.Geometry(
                  OfflineGeometryRegionDefinition(
                    "custom://style.json",
                    "{\"type\":\"Point\",\"coordinates\":[2,1]}".encodeToByteArray(),
                    0.0,
                    1.0,
                    1.0f,
                    false,
                  )
                )
              ),
              ByteArray(0),
            )
            .await()
        assertTrue(region.id > 0)
      }
  }

  @Test
  fun offlineRegionsListCompletesAndConsumesOperation(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault().copy(cachePath = ":memory:"))
      .use { runtime -> assertTrue(runtime.offlineRegionsList().await().isEmpty()) }
  }

  @Test
  fun resourceProviderSeesSchemeAliasAndItsResolvedUrl(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      val resolvedUrl = AtomicReference<String?>(null)
      runtime
        .setResourceProvider(
          ResourceProvider(
            callback = provider@{ request, handle ->
                if (request.requestedUrl != "maplibre://maps/style") {
                  return@provider ResourceProviderDecision.PASS_THROUGH
                }
                resolvedUrl.store(request.resolvedUrl)
                handle.resourceRequestComplete(
                  ResourceResponse(ResourceResponseStatus.OK)
                    .copy(bytes = EMPTY_STYLE_JSON.encodeToByteArray())
                )
                ResourceProviderDecision.HANDLE
              }
          )
        )
        .await()
      val map =
        runtime
          .mapCreate(
            GeneratedApi.mapOptionsDefault()
              .copy(
                initialExtent =
                  GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
              )
          )
          .await()
      try {
        map.setStyleUrl("maplibre://maps/style").await()
        assertTrue(waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED))
        assertEquals("https://demotiles.maplibre.org/style.json", resolvedUrl.load())
      } finally {
        map.release()
      }
    }
  }

  @Test
  fun resourceProviderCompletesStyleRequestThroughRuntime(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      val calls = AtomicInt(0)
      val callbackError = AtomicReference<Throwable?>(null)
      runtime
        .setResourceProvider(
          ResourceProvider(
            callback = provider@{ request, handle ->
                try {
                  if (request.requestedUrl != "custom://style.json") {
                    return@provider ResourceProviderDecision.PASS_THROUGH
                  }
                  calls.addAndFetch(1)
                  assertEquals(ResourceKind.STYLE, request.kind)
                  handle.resourceRequestComplete(
                    ResourceResponse(ResourceResponseStatus.OK)
                      .copy(bytes = EMPTY_STYLE_JSON.encodeToByteArray())
                  )
                  ResourceProviderDecision.HANDLE
                } catch (error: Throwable) {
                  callbackError.store(error)
                  throw error
                }
              }
          )
        )
        .await()
      val map =
        runtime
          .mapCreate(
            GeneratedApi.mapOptionsDefault()
              .copy(
                initialExtent =
                  GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
              )
          )
          .await()
      try {
        map.setStyleUrl("custom://style.json").await()
        val event = waitForMapEventRecord(runtime, map, RuntimeEventType.MAP_STYLE_LOADED)
        val copiedMessage = event.message
        assertEquals(RuntimeEventSourceType.MAP, event.sourceType)
        assertTrue(event.source != 0uL)
        assertEquals(RuntimeEventPayload.None, event.payload)
        // A drained value stays readable after the next drain ends the batch window.
        runtime.drainEvents().use { it.get().events }
        assertEquals(copiedMessage, event.message)
        callbackError.load()?.let { throw AssertionError("resource provider callback failed", it) }
        assertEquals(1, calls.load())
      } finally {
        map.release()
      }
    }
  }

  @Test
  fun handledResourceRequestCanCompleteAfterTheProviderReturns(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      val handledRequest = AtomicReference<ResourceRequestHandle?>(null)
      runtime
        .setResourceProvider(
          ResourceProvider(
            callback = provider@{ request, handle ->
                if (request.requestedUrl != "custom://deferred-style.json") {
                  return@provider ResourceProviderDecision.PASS_THROUGH
                }
                handledRequest.store(handle)
                ResourceProviderDecision.HANDLE
              }
          )
        )
        .await()
      val map =
        runtime
          .mapCreate(
            GeneratedApi.mapOptionsDefault()
              .copy(
                initialExtent =
                  GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
              )
          )
          .await()
      try {
        map.setStyleUrl("custom://deferred-style.json").await()
        val handle = waitForHandledRequest(runtime, handledRequest)
        assertFalse(handle.resourceRequestCancelled())
        handle.resourceRequestComplete(
          ResourceResponse(ResourceResponseStatus.OK)
            .copy(bytes = EMPTY_STYLE_JSON.encodeToByteArray())
        )
        assertFalse(handle.resourceRequestCancelled())
        assertFailsWith<InvalidStateException> {
          handle.resourceRequestComplete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
        }
        assertTrue(waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED))
      } finally {
        map.release()
      }
    }
  }

  @Test
  fun resourceErrorBecomesACopiedMapLoadingFailureEvent(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      runtime
        .setResourceProvider(
          ResourceProvider(
            callback = provider@{ request, handle ->
                if (request.requestedUrl != "custom://error-style.json") {
                  return@provider ResourceProviderDecision.PASS_THROUGH
                }
                handle.resourceRequestComplete(
                  ResourceResponse(ResourceResponseStatus.ERROR)
                    .copy(
                      errorReason = ResourceErrorReason.NOT_FOUND,
                      errorMessage = "custom style failed",
                    )
                )
                ResourceProviderDecision.HANDLE
              }
          )
        )
        .await()
      val map =
        runtime
          .mapCreate(
            GeneratedApi.mapOptionsDefault()
              .copy(
                initialExtent =
                  GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
              )
          )
          .await()
      try {
        map.setStyleUrl("custom://error-style.json").await()
        val event = waitForMapEventRecord(runtime, map, RuntimeEventType.MAP_LOADING_FAILED)
        val copiedMessage = event.message
        assertEquals(RuntimeEventSourceType.MAP, event.sourceType)
        assertTrue(event.source != 0uL)
        assertTrue(copiedMessage.contains("custom style failed"))
        // A drained value stays readable after the next drain ends the batch window.
        runtime.drainEvents().use { it.get().events }
        assertEquals(copiedMessage, event.message)
      } finally {
        map.release()
      }
    }
  }

  @Test
  fun closingAMapCancelsItsOutstandingResourceRequest(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      val handledRequest = AtomicReference<ResourceRequestHandle?>(null)
      runtime
        .setResourceProvider(
          ResourceProvider(
            callback = provider@{ request, handle ->
                if (request.requestedUrl != "custom://cancelled-style.json") {
                  return@provider ResourceProviderDecision.PASS_THROUGH
                }
                handledRequest.store(handle)
                ResourceProviderDecision.HANDLE
              }
          )
        )
        .await()
      val map =
        runtime
          .mapCreate(
            GeneratedApi.mapOptionsDefault()
              .copy(
                initialExtent =
                  GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
              )
          )
          .await()
      map.setStyleUrl("custom://cancelled-style.json").await()
      val handle = waitForHandledRequest(runtime, handledRequest)

      map.release()

      assertTrue(waitForRequestCancellation(runtime, handle))
      assertFailsWith<InvalidStateException> {
        handle.resourceRequestComplete(
          ResourceResponse(ResourceResponseStatus.OK)
            .copy(bytes = EMPTY_STYLE_JSON.encodeToByteArray())
        )
      }
      handle.close()
    }
  }

  @Test
  fun cancelCallbackRunsOnceWhenTheMapDiscardsItsRequest(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      val handledRequest = captureHandledRequest(runtime, "custom://cancel-callback-style.json")
      val map = createSmallMap(runtime)
      map.setStyleUrl("custom://cancel-callback-style.json").await()
      val handle = waitForHandledRequest(runtime, handledRequest)
      val cancels = AtomicInt(0)
      val rejectedCancels = AtomicInt(0)
      handle.resourceRequestSetCancelCallback { cancels.addAndFetch(1) }
      // The request keeps its first callback.
      assertFailsWith<InvalidStateException> {
        handle.resourceRequestSetCancelCallback { rejectedCancels.addAndFetch(1) }
      }

      map.release().await()

      assertTrue(waitForCondition { cancels.load() == 1 })
      assertTrue(handle.resourceRequestCancelled())
      repeat(CANCEL_SETTLE_ROUNDS) {
        runtime.barrier().await()
        sleepMillis(1)
      }
      assertEquals(1, cancels.load())
      assertEquals(0, rejectedCancels.load())

      handle.close()

      assertFailsWith<InvalidStateException> { handle.resourceRequestSetCancelCallback {} }
    }
  }

  @Test
  fun cancelCallbackMayCloseItsOwnRequest(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      val handledRequest = captureHandledRequest(runtime, "custom://self-closing-style.json")
      val map = createSmallMap(runtime)
      map.setStyleUrl("custom://self-closing-style.json").await()
      val handle = waitForHandledRequest(runtime, handledRequest)
      val closes = AtomicInt(0)
      handle.resourceRequestSetCancelCallback {
        handle.close()
        closes.addAndFetch(1)
      }

      map.release().await()

      assertTrue(waitForCondition { closes.load() == 1 })
      assertFailsWith<InvalidStateException> { handle.resourceRequestCancelled() }
      handle.close()
    }
  }

  @Test
  fun lateCancelRegistrationReportsCancellationWithoutInvokingCallback(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      val handledRequest =
        captureHandledRequest(runtime, "custom://late-cancel-callback-style.json")
      val map = createSmallMap(runtime)
      map.setStyleUrl("custom://late-cancel-callback-style.json").await()
      val handle = waitForHandledRequest(runtime, handledRequest)
      map.release().await()
      assertTrue(waitForRequestCancellation(runtime, handle))

      val cancels = AtomicInt(0)
      assertTrue(handle.resourceRequestSetCancelCallback { cancels.addAndFetch(1) })
      assertEquals(0, cancels.load())
      assertFailsWith<InvalidStateException> {
        handle.resourceRequestComplete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
      }
      handle.close()
    }
  }

  @Test
  fun cancelCallbackStaysUninvokedForACompletedRequest(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      val handledRequest = captureHandledRequest(runtime, "custom://completed-cancel-style.json")
      val map = createSmallMap(runtime)
      map.setStyleUrl("custom://completed-cancel-style.json").await()
      val handle = waitForHandledRequest(runtime, handledRequest)
      val cancels = AtomicInt(0)
      handle.resourceRequestSetCancelCallback { cancels.addAndFetch(1) }
      handle.resourceRequestComplete(
        ResourceResponse(ResourceResponseStatus.OK)
          .copy(bytes = EMPTY_STYLE_JSON.encodeToByteArray())
      )
      assertTrue(waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED))

      // MapLibre runs its cancel hook on every request teardown, including a completed one.
      map.release().await()

      repeat(CANCEL_SETTLE_ROUNDS) {
        runtime.barrier().await()
        sleepMillis(1)
      }
      assertEquals(0, cancels.load())
    }
  }

  @Test
  fun resourceProviderIsConsultedUntilClearedWhileMapIsLive(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      val firstCalls = AtomicInt(0)
      val secondCalls = AtomicInt(0)
      runtime
        .setResourceProvider(
          ResourceProvider(
            callback = provider@{ _, _ ->
                firstCalls.addAndFetch(1)
                ResourceProviderDecision.PASS_THROUGH
              }
          )
        )
        .await()
      val map =
        runtime
          .mapCreate(
            GeneratedApi.mapOptionsDefault()
              .copy(
                initialExtent =
                  GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
              )
          )
          .await()
      try {
        loadUnservedStyle(runtime, map, "jar:file:/packaged/first.json")
        assertTrue(firstCalls.load() > 0)

        runtime
          .setResourceProvider(
            ResourceProvider(
              callback = provider@{ _, _ ->
                  secondCalls.addAndFetch(1)
                  ResourceProviderDecision.PASS_THROUGH
                }
            )
          )
          .await()
        val firstCallsAfterReplace = firstCalls.load()
        loadUnservedStyle(runtime, map, "jar:file:/packaged/second.json")
        assertTrue(secondCalls.load() > 0)
        assertEquals(firstCallsAfterReplace, firstCalls.load())

        runtime.clearResourceProvider().await()
        val secondCallsAfterClear = secondCalls.load()
        loadUnservedStyle(runtime, map, "jar:file:/packaged/third.json")
        assertEquals(firstCallsAfterReplace, firstCalls.load())
        assertEquals(secondCallsAfterClear, secondCalls.load())

        // Clearing an already cleared provider stays a successful no-op.
        runtime.clearResourceProvider().await()
      } finally {
        map.release()
      }
    }
  }

  @Test
  fun resourceTransformRewritesStyleRequestsUntilCleared(): Unit = runSuspendTest {
    val previousNetworkStatus = GeneratedApi.networkStatusGet()
    GeneratedApi.networkStatusSet(NetworkStatus.ONLINE)
    try {
      GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
        val calls = AtomicInt(0)
        val lastUrl = AtomicReference<String?>(null)
        val lastKind = AtomicReference<ResourceKind?>(null)
        runtime
          .setResourceTransform(
            ResourceTransform(
              callback = transform@{ kind, url, response ->
                  calls.addAndFetch(1)
                  lastUrl.store(url)
                  lastKind.store(kind)
                  GeneratedApi.resourceTransformResponseSetUrl(
                    response,
                    "unsupported://rewritten-style.json",
                  )
                }
            )
          )
          .await()
        val map =
          runtime
            .mapCreate(
              GeneratedApi.mapOptionsDefault()
                .copy(
                  initialExtent =
                    GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
                )
            )
            .await()
        try {
          map.setStyleUrl("http://example.invalid/original-style.json").await()
          waitForMapLoadingFailure(runtime, map)
          val callsBeforeClear = calls.load()
          assertTrue(callsBeforeClear > 0)
          assertEquals("http://example.invalid/original-style.json", lastUrl.load())
          assertEquals(ResourceKind.STYLE, lastKind.load())

          runtime.clearResourceTransform().await()
          map.setStyleUrl("unsupported://after-clear-style.json").await()
          waitForMapLoadingFailure(runtime, map, "unsupported://after-clear-style.json")
          assertEquals(callsBeforeClear, calls.load())
        } finally {
          map.release().await()
        }
      }
    } finally {
      GeneratedApi.networkStatusSet(previousNetworkStatus)
    }
  }

  @Test
  fun passThroughResourceRequestExpiresAfterTheProviderReturns(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      val requestHandle = AtomicReference<ResourceRequestHandle?>(null)
      runtime
        .setResourceProvider(
          ResourceProvider(
            callback = provider@{ request, handle ->
                if (request.requestedUrl == "custom://pass-through-style.json") {
                  requestHandle.store(handle)
                }
                ResourceProviderDecision.PASS_THROUGH
              }
          )
        )
        .await()
      val map =
        runtime
          .mapCreate(
            GeneratedApi.mapOptionsDefault()
              .copy(
                initialExtent =
                  GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
              )
          )
          .await()
      try {
        map.setStyleUrl("custom://pass-through-style.json").await()
        val handle = waitForHandledRequest(runtime, requestHandle)
        assertEquals(
          RuntimeEventType.MAP_LOADING_FAILED,
          waitForMapEventRecord(runtime, map, RuntimeEventType.MAP_LOADING_FAILED).type,
        )
        assertFailsWith<InvalidStateException> { handle.resourceRequestCancelled() }
        assertFailsWith<InvalidStateException> {
          handle.resourceRequestComplete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
        }
      } finally {
        map.release()
      }
    }
  }

  @Test
  fun runtimeCloseDuringResourceProviderCallbackRejectsBeforeNativeDestroy(): Unit =
    runSuspendTest {
      GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
        val closeError = AtomicReference<Throwable?>(null)
        runtime
          .setResourceProvider(
            ResourceProvider(
              callback = provider@{ request, _ ->
                  if (request.requestedUrl == "custom://close-during-provider.json") {
                    closeError.store(assertFailsWith<InvalidStateException> { runtime.release() })
                  }
                  ResourceProviderDecision.PASS_THROUGH
                }
            )
          )
          .await()
        val map =
          runtime
            .mapCreate(
              GeneratedApi.mapOptionsDefault()
                .copy(
                  initialExtent =
                    GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
                )
            )
            .await()
        try {
          map.setStyleUrl("custom://close-during-provider.json").await()
          assertTrue(waitForCondition { closeError.load() != null })
          assertFalse(runtime.isClosed)
        } finally {
          map.release()
        }
      }
    }

  @Test
  fun runtimeCloseDuringResourceTransformCallbackRejectsBeforeNativeDestroy(): Unit =
    runSuspendTest {
      GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
        val closeError = AtomicReference<Throwable?>(null)
        runtime
          .setResourceTransform(
            ResourceTransform(
              callback = transform@{ kind, url, response ->
                  if (url == "http://example.invalid/close-during-transform.json") {
                    closeError.store(assertFailsWith<InvalidStateException> { runtime.release() })
                  }
                  GeneratedApi.resourceTransformResponseSetUrl(
                    response,
                    "unsupported://close-during-transform.json",
                  )
                }
            )
          )
          .await()
        val map =
          runtime
            .mapCreate(
              GeneratedApi.mapOptionsDefault()
                .copy(
                  initialExtent =
                    GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
                )
            )
            .await()
        try {
          map.setStyleUrl("http://example.invalid/close-during-transform.json").await()
          assertTrue(waitForCondition { closeError.load() != null })
          assertFalse(runtime.isClosed)
        } finally {
          map.release()
        }
      }
    }

  private suspend fun waitForMapEvent(
    runtime: RuntimeHandle,
    map: MapHandle,
    type: RuntimeEventType,
  ): Boolean {
    repeat(10_000) {
      runtime.barrier().await()
      if (
        runtime
          .drainEvents()
          .use { it.get().events }
          .any { it.type == type && it.sourceType == RuntimeEventSourceType.MAP }
      )
        return true
      runtime.barrier().await()
      sleepMillis(1)
    }
    return false
  }

  private suspend fun waitForMapEventRecord(
    runtime: RuntimeHandle,
    map: MapHandle,
    type: RuntimeEventType,
  ): RuntimeEvent {
    repeat(10_000) {
      runtime.barrier().await()
      for (event in runtime.drainEvents().use { it.get().events }) {
        if (event.type == type && event.sourceType == RuntimeEventSourceType.MAP) return event
      }
      runtime.barrier().await()
      sleepMillis(1)
    }
    error("runtime event $type did not arrive")
  }

  /** Installs a provider that handles [url] without completing it and hands the request out. */
  private suspend fun captureHandledRequest(
    runtime: RuntimeHandle,
    url: String,
  ): AtomicReference<ResourceRequestHandle?> {
    val handledRequest = AtomicReference<ResourceRequestHandle?>(null)
    runtime
      .setResourceProvider(
        ResourceProvider(
          callback = provider@{ request, handle ->
              if (request.requestedUrl != url) {
                return@provider ResourceProviderDecision.PASS_THROUGH
              }
              handledRequest.store(handle)
              ResourceProviderDecision.HANDLE
            }
        )
      )
      .await()
    return handledRequest
  }

  private suspend fun createSmallMap(runtime: RuntimeHandle): MapHandle =
    runtime
      .mapCreate(
        GeneratedApi.mapOptionsDefault()
          .copy(
            initialExtent =
              GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
          )
      )
      .await()

  private suspend fun waitForHandledRequest(
    runtime: RuntimeHandle,
    handledRequest: AtomicReference<ResourceRequestHandle?>,
  ): ResourceRequestHandle {
    repeat(10_000) {
      handledRequest.load()?.let {
        return it
      }
      runtime.barrier().await()
      sleepMillis(1)
    }
    error("resource provider did not receive handled request")
  }

  private suspend fun waitForRequestCancellation(
    runtime: RuntimeHandle,
    handle: ResourceRequestHandle,
  ): Boolean {
    repeat(10_000) {
      if (handle.resourceRequestCancelled()) return true
      runtime.barrier().await()
      sleepMillis(1)
    }
    return false
  }

  /**
   * Loads a style URL whose scheme no file source serves; the failure names the scheme and URL,
   * proving the request reached the network file source.
   */
  private suspend fun loadUnservedStyle(runtime: RuntimeHandle, map: MapHandle, styleUrl: String) {
    map.setStyleUrl(styleUrl).await()
    val message = waitForMapLoadingFailure(runtime, map, styleUrl)
    assertTrue(message.contains("\"jar\""), "unexpected loading failure message: $message")
  }

  private suspend fun waitForMapLoadingFailure(
    runtime: RuntimeHandle,
    map: MapHandle,
    styleUrl: String? = null,
  ): String {
    repeat(10_000) {
      runtime.barrier().await()
      for (event in runtime.drainEvents().use { it.get().events }) {
        if (
          event.type == RuntimeEventType.MAP_LOADING_FAILED &&
            event.sourceType == RuntimeEventSourceType.MAP &&
            (styleUrl == null || event.message.contains(styleUrl))
        ) {
          return event.message
        }
      }
      runtime.barrier().await()
      sleepMillis(1)
    }
    error("map loading failure for $styleUrl did not arrive")
  }

  private suspend fun waitForCondition(condition: suspend () -> Boolean): Boolean {
    repeat(10_000) {
      if (condition()) return true
      sleepMillis(1)
    }
    return false
  }
}

private const val CANCEL_SETTLE_ROUNDS = 50
