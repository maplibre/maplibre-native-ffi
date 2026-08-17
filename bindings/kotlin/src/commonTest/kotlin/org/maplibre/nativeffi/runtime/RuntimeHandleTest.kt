package org.maplibre.nativeffi.runtime

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState
import org.maplibre.nativeffi.resource.ResourceErrorReason
import org.maplibre.nativeffi.resource.ResourceKind
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceProviderDecision
import org.maplibre.nativeffi.resource.ResourceRequestHandle
import org.maplibre.nativeffi.resource.ResourceResponse
import org.maplibre.nativeffi.resource.ResourceResponseStatus
import org.maplibre.nativeffi.resource.ResourceTransformCallback

@OptIn(ExperimentalAtomicApi::class)
class RuntimeHandleTest {
  @Test
  fun runtimeRunsOnceAndCloses(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val runtime = RuntimeHandle.create(RuntimeOptions())

      assertFalse(runtime.isClosed)
      runtime.barrier()
      runtime.close()
      runtime.close()

      assertTrue(runtime.isClosed)
      assertFailsWith<InvalidStateException> { runSuspendTest { runtime.barrier() } }
    }

  @Test
  fun notificationCallbackUsesSharedReceiverDrain(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      RuntimeHandle.create(RuntimeOptions()).use { runtime ->
        val callbacks = AtomicInt(0)
        runtime.setNotificationCallback { callbacks.addAndFetch(1) }

        runtime.barrier()
        assertTrue(waitForCondition { callbacks.load() > 0 })

        runtime.clearNotificationCallback()
        val afterClear = callbacks.load()
        runtime.barrier()
        waitForAsyncTestWork()
        assertEquals(afterClear, callbacks.load())
      }
    }

  @Test
  fun freshRuntimeDrainsAnEmptyBatch(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      RuntimeHandle.create(RuntimeOptions()).use { runtime ->
        val batch = runtime.drainEvents()
        assertEquals(emptyList(), batch.events)
      }
    }

  @Test
  fun ambientCacheOperationRetainsRuntimeUntilClosed(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val runtime = RuntimeHandle.create(RuntimeOptions())
      val operation = runtime.startAmbientCacheOperation(AmbientCacheOperation.INVALIDATE)

      assertFalse(operation.isClosed)
      assertFailsWith<InvalidStateException> { runSuspendTest { runtime.close() } }

      operation.close()
      operation.close()

      assertTrue(operation.isClosed)
      runtime.close()
      assertTrue(runtime.isClosed)
    }

  @Test
  fun setMaximumAmbientCacheSizeReachesNativeAndRejectsNegativeSize(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val runtime = RuntimeHandle.create(RuntimeOptions().apply { cachePath = ":memory:" })
      val operation = runtime.startSetMaximumAmbientCacheSize(8L shl 20)

      assertFalse(operation.isClosed)
      operation.close()

      // Binding-owned validation fails before crossing into C.
      assertFailsWith<InvalidArgumentException> { runtime.startSetMaximumAmbientCacheSize(-1L) }
      runtime.close()
    }

  @Test
  fun offlineDownloadStateUnknownRawValueRejectsBeforeNativeCall(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      RuntimeHandle.create(RuntimeOptions()).use { runtime ->
        assertFailsWith<InvalidArgumentException> {
          runtime.startSetOfflineRegionDownloadState(1, OfflineRegionDownloadState(900))
        }
      }
    }

  @Test
  fun geometryOfflineRegionDefinitionStartsOperation(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      RuntimeHandle.create(RuntimeOptions().apply { cachePath = ":memory:" }).use { runtime ->
        val operation =
          runtime.startCreateOfflineRegion(
            OfflineRegionDefinition.GeometryRegion(
              "custom://style.json",
              "{\"type\":\"Point\",\"coordinates\":[2,1]}".encodeToByteArray(),
              0.0,
              1.0,
              1.0f,
              false,
            ),
            ByteArray(0),
          )
        operation.close()
        assertTrue(operation.isClosed)
      }
    }

  @Test
  fun offlineRegionsListCompletesAndConsumesOperation(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      RuntimeHandle.create(RuntimeOptions().apply { cachePath = ":memory:" }).use { runtime ->
        val operation = runtime.startOfflineRegions()

        waitForOperation(runtime, operation)
        assertTrue(operation.waitForCompletion(0))
        assertEquals(MaplibreStatus.OK, operation.terminalStatus())
        assertEquals("", operation.diagnostic())

        assertTrue(runtime.takeOfflineRegionsResult(operation).isEmpty())
        assertTrue(operation.isClosed)
        assertFailsWith<InvalidStateException> { runtime.takeOfflineRegionsResult(operation) }
      }
    }

  // BND-155.
  @Test
  fun resourceProviderSeesSchemeAliasAndItsResolvedUrl(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      RuntimeHandle.create(RuntimeOptions()).use { runtime ->
        val resolvedUrl = AtomicReference<String?>(null)
        runtime.setResourceProvider(
          ResourceProviderCallback { request, handle ->
            if (request.requestedUrl != "maplibre://maps/style") {
              return@ResourceProviderCallback ResourceProviderDecision.PASS_THROUGH
            }
            resolvedUrl.store(request.resolvedUrl)
            handle.complete(
              ResourceResponse(ResourceResponseStatus.OK).apply {
                bytes = STYLE_JSON.encodeToByteArray()
              }
            )
            ResourceProviderDecision.HANDLE
          }
        )
        val map =
          MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
            },
          )
        try {
          map.setStyleUrl("maplibre://maps/style")
          assertTrue(waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED))
          assertEquals("https://demotiles.maplibre.org/style.json", resolvedUrl.load())
        } finally {
          map.close()
        }
      }
    }

  @Test
  fun resourceProviderCompletesStyleRequestThroughRuntime(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      RuntimeHandle.create(RuntimeOptions()).use { runtime ->
        val calls = AtomicInt(0)
        val callbackError = AtomicReference<Throwable?>(null)
        runtime.setResourceProvider(
          ResourceProviderCallback { request, handle ->
            try {
              if (request.requestedUrl != "custom://style.json") {
                return@ResourceProviderCallback ResourceProviderDecision.PASS_THROUGH
              }
              calls.addAndFetch(1)
              assertEquals(ResourceKind.STYLE, request.kind)
              handle.complete(
                ResourceResponse(ResourceResponseStatus.OK).apply {
                  bytes = STYLE_JSON.encodeToByteArray()
                }
              )
              ResourceProviderDecision.HANDLE
            } catch (error: Throwable) {
              callbackError.store(error)
              throw error
            }
          }
        )
        val map =
          MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
            },
          )
        try {
          map.setStyleUrl("custom://style.json")
          val event = waitForMapEventRecord(runtime, map, RuntimeEventType.MAP_STYLE_LOADED)
          val copiedMessage = event.message
          assertEquals(RuntimeEventSourceType.MAP, event.sourceType)
          assertEquals(map, event.mapSource)
          assertNull(event.runtimeSource)
          assertEquals(RuntimeEventPayload.None, event.payload)
          // A drained value stays readable after the next drain ends the batch window.
          runtime.drainEvents()
          assertEquals(copiedMessage, event.message)
          callbackError.load()?.let {
            throw AssertionError("resource provider callback failed", it)
          }
          assertEquals(1, calls.load())
        } finally {
          map.close()
        }
      }
    }

  @Test
  fun handledResourceRequestCanCompleteAfterTheProviderReturns(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      RuntimeHandle.create(RuntimeOptions()).use { runtime ->
        val handledRequest = AtomicReference<ResourceRequestHandle?>(null)
        runtime.setResourceProvider(
          ResourceProviderCallback { request, handle ->
            if (request.requestedUrl != "custom://deferred-style.json") {
              return@ResourceProviderCallback ResourceProviderDecision.PASS_THROUGH
            }
            handledRequest.store(handle)
            ResourceProviderDecision.HANDLE
          }
        )
        val map =
          MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
            },
          )
        try {
          map.setStyleUrl("custom://deferred-style.json")
          val handle = waitForHandledRequest(runtime, handledRequest)
          assertFalse(handle.isCancelled())
          handle.complete(
            ResourceResponse(ResourceResponseStatus.OK).apply {
              bytes = STYLE_JSON.encodeToByteArray()
            }
          )
          assertFailsWith<InvalidStateException> { handle.isCancelled() }
          assertFailsWith<InvalidStateException> {
            handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
          }
          assertTrue(waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED))
        } finally {
          map.close()
        }
      }
    }

  @Test
  fun resourceErrorBecomesACopiedMapLoadingFailureEvent(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      RuntimeHandle.create(RuntimeOptions()).use { runtime ->
        runtime.setResourceProvider(
          ResourceProviderCallback { request, handle ->
            if (request.requestedUrl != "custom://error-style.json") {
              return@ResourceProviderCallback ResourceProviderDecision.PASS_THROUGH
            }
            handle.complete(
              ResourceResponse(ResourceResponseStatus.ERROR).apply {
                errorReason = ResourceErrorReason.NOT_FOUND
                errorMessage = "custom style failed"
              }
            )
            ResourceProviderDecision.HANDLE
          }
        )
        val map =
          MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
            },
          )
        try {
          map.setStyleUrl("custom://error-style.json")
          val event = waitForMapEventRecord(runtime, map, RuntimeEventType.MAP_LOADING_FAILED)
          val copiedMessage = event.message
          assertEquals(RuntimeEventSourceType.MAP, event.sourceType)
          assertEquals(map, event.mapSource)
          assertNull(event.runtimeSource)
          assertTrue(copiedMessage.contains("custom style failed"))
          // A drained value stays readable after the next drain ends the batch window.
          runtime.drainEvents()
          assertEquals(copiedMessage, event.message)
        } finally {
          map.close()
        }
      }
    }

  @Test
  fun closingAMapCancelsItsOutstandingResourceRequest(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      RuntimeHandle.create(RuntimeOptions()).use { runtime ->
        val handledRequest = AtomicReference<ResourceRequestHandle?>(null)
        runtime.setResourceProvider(
          ResourceProviderCallback { request, handle ->
            if (request.requestedUrl != "custom://cancelled-style.json") {
              return@ResourceProviderCallback ResourceProviderDecision.PASS_THROUGH
            }
            handledRequest.store(handle)
            ResourceProviderDecision.HANDLE
          }
        )
        val map =
          MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
            },
          )
        map.setStyleUrl("custom://cancelled-style.json")
        val handle = waitForHandledRequest(runtime, handledRequest)

        map.close()

        assertTrue(waitForRequestCancellation(runtime, handle))
        assertFailsWith<InvalidStateException> {
          handle.complete(
            ResourceResponse(ResourceResponseStatus.OK).apply {
              bytes = STYLE_JSON.encodeToByteArray()
            }
          )
        }
        handle.close()
      }
    }

  // BND-154.
  @Test
  fun resourceProviderIsConsultedUntilClearedWhileMapIsLive(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      RuntimeHandle.create(RuntimeOptions()).use { runtime ->
        val firstCalls = AtomicInt(0)
        val secondCalls = AtomicInt(0)
        runtime.setResourceProvider(
          ResourceProviderCallback { _, _ ->
            firstCalls.addAndFetch(1)
            ResourceProviderDecision.PASS_THROUGH
          }
        )
        val map =
          MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
            },
          )
        try {
          loadUnservedStyle(runtime, map, "jar:file:/packaged/first.json")
          assertTrue(firstCalls.load() > 0)

          runtime.setResourceProvider(
            ResourceProviderCallback { _, _ ->
              secondCalls.addAndFetch(1)
              ResourceProviderDecision.PASS_THROUGH
            }
          )
          val firstCallsAfterReplace = firstCalls.load()
          loadUnservedStyle(runtime, map, "jar:file:/packaged/second.json")
          assertTrue(secondCalls.load() > 0)
          assertEquals(firstCallsAfterReplace, firstCalls.load())

          runtime.clearResourceProvider()
          val secondCallsAfterClear = secondCalls.load()
          loadUnservedStyle(runtime, map, "jar:file:/packaged/third.json")
          assertEquals(firstCallsAfterReplace, firstCalls.load())
          assertEquals(secondCallsAfterClear, secondCalls.load())

          // Clearing an already cleared provider stays a successful no-op.
          runtime.clearResourceProvider()
        } finally {
          map.close()
        }
      }
    }

  @Test
  fun resourceTransformRewritesStyleRequestsUntilCleared(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val previousNetworkStatus = Maplibre.networkStatus
      Maplibre.setNetworkStatus(NetworkStatus.ONLINE)
      try {
        RuntimeHandle.create(RuntimeOptions()).use { runtime ->
          val calls = AtomicInt(0)
          val lastUrl = AtomicReference<String?>(null)
          val lastKind = AtomicReference<ResourceKind?>(null)
          runtime.setResourceTransform(
            ResourceTransformCallback { request ->
              calls.addAndFetch(1)
              lastUrl.store(request.url)
              lastKind.store(request.kind)
              "unsupported://rewritten-style.json"
            }
          )
          val map =
            MapHandle.create(
              runtime,
              MapOptions().apply {
                width = 64
                height = 64
              },
            )
          try {
            map.setStyleUrl("http://example.invalid/original-style.json")
            assertTrue(
              waitForCondition {
                runtime.barrier()
                calls.load() > 0
              }
            )
            assertEquals(1, calls.load())
            assertEquals("http://example.invalid/original-style.json", lastUrl.load())
            assertEquals(ResourceKind.STYLE, lastKind.load())

            runtime.clearResourceTransform()
            map.setStyleUrl("unsupported://after-clear-style.json")
            repeat(100) {
              runtime.barrier()
              // Keep native loading moving while proving the retired transform stays retired.
              runtime.drainEvents()
              assertEquals(1, calls.load())
            }
          } finally {
            map.close()
          }
        }
      } finally {
        Maplibre.setNetworkStatus(previousNetworkStatus)
      }
    }

  @Test
  fun passThroughResourceRequestExpiresAfterTheProviderReturns(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      RuntimeHandle.create(RuntimeOptions()).use { runtime ->
        val requestHandle = AtomicReference<ResourceRequestHandle?>(null)
        runtime.setResourceProvider(
          ResourceProviderCallback { request, handle ->
            if (request.requestedUrl == "custom://pass-through-style.json") {
              requestHandle.store(handle)
            }
            ResourceProviderDecision.PASS_THROUGH
          }
        )
        val map =
          MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
            },
          )
        try {
          map.setStyleUrl("custom://pass-through-style.json")
          val handle = waitForHandledRequest(runtime, requestHandle)
          assertEquals(
            RuntimeEventType.MAP_LOADING_FAILED,
            waitForMapEventRecord(runtime, map, RuntimeEventType.MAP_LOADING_FAILED).type,
          )
          assertFailsWith<InvalidStateException> { handle.isCancelled() }
          assertFailsWith<InvalidStateException> {
            handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
          }
        } finally {
          map.close()
        }
      }
    }

  @Test
  fun runtimeCloseDuringResourceProviderCallbackRejectsBeforeNativeDestroy(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      RuntimeHandle.create(RuntimeOptions()).use { runtime ->
        val closeError = AtomicReference<Throwable?>(null)
        runtime.setResourceProvider(
          ResourceProviderCallback { request, _ ->
            if (request.requestedUrl == "custom://close-during-provider.json") {
              closeError.store(
                assertFailsWith<InvalidStateException> { runSuspendTest { runtime.close() } }
              )
            }
            ResourceProviderDecision.PASS_THROUGH
          }
        )
        val map =
          MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
            },
          )
        try {
          map.setStyleUrl("custom://close-during-provider.json")
          assertTrue(
            waitForCondition {
              runtime.barrier()
              closeError.load() != null
            }
          )
          assertFalse(runtime.isClosed)
        } finally {
          map.close()
        }
      }
    }

  @Test
  fun runtimeCloseDuringResourceTransformCallbackRejectsBeforeNativeDestroy(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      RuntimeHandle.create(RuntimeOptions()).use { runtime ->
        val closeError = AtomicReference<Throwable?>(null)
        runtime.setResourceTransform(
          ResourceTransformCallback { request ->
            if (request.url == "http://example.invalid/close-during-transform.json") {
              closeError.store(
                assertFailsWith<InvalidStateException> { runSuspendTest { runtime.close() } }
              )
            }
            "unsupported://close-during-transform.json"
          }
        )
        val map =
          MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
            },
          )
        try {
          map.setStyleUrl("http://example.invalid/close-during-transform.json")
          assertTrue(
            waitForCondition {
              runtime.barrier()
              closeError.load() != null
            }
          )
          assertFalse(runtime.isClosed)
        } finally {
          map.close()
        }
      }
    }

  private suspend fun waitForOperation(runtime: RuntimeHandle, operation: OperationHandle<*>) {
    repeat(10_000) {
      if (operation.poll()) {
        return
      }
      runtime.barrier()
      waitForAsyncTestWork()
    }
    error("operation did not complete")
  }

  private suspend fun waitForMapEvent(
    runtime: RuntimeHandle,
    map: MapHandle,
    type: RuntimeEventType,
  ): Boolean {
    repeat(10_000) {
      runtime.barrier()
      if (runtime.drainEvents().events.any { it.type == type && it.mapSource == map }) return true
      runtime.barrier()
      waitForAsyncTestWork()
    }
    return false
  }

  private suspend fun waitForMapEventRecord(
    runtime: RuntimeHandle,
    map: MapHandle,
    type: RuntimeEventType,
  ): RuntimeEvent {
    repeat(10_000) {
      runtime.barrier()
      for (event in runtime.drainEvents().events) {
        if (event.type == type && event.mapSource == map) return event
      }
      runtime.barrier()
      waitForAsyncTestWork()
    }
    error("runtime event $type did not arrive")
  }

  private suspend fun waitForHandledRequest(
    runtime: RuntimeHandle,
    handledRequest: AtomicReference<ResourceRequestHandle?>,
  ): ResourceRequestHandle {
    repeat(10_000) {
      handledRequest.load()?.let {
        return it
      }
      runtime.barrier()
      waitForAsyncTestWork()
    }
    error("resource provider did not receive handled request")
  }

  private suspend fun waitForRequestCancellation(
    runtime: RuntimeHandle,
    handle: ResourceRequestHandle,
  ): Boolean {
    repeat(10_000) {
      if (handle.isCancelled()) return true
      runtime.barrier()
      waitForAsyncTestWork()
    }
    return false
  }

  /**
   * Loads a style URL whose scheme no file source serves; the failure names the scheme and URL,
   * proving the request reached the network file source.
   */
  private suspend fun loadUnservedStyle(runtime: RuntimeHandle, map: MapHandle, styleUrl: String) {
    map.setStyleUrl(styleUrl)
    val message = waitForMapLoadingFailure(runtime, map, styleUrl)
    assertTrue(message.contains("\"jar\""), "unexpected loading failure message: $message")
  }

  private suspend fun waitForMapLoadingFailure(
    runtime: RuntimeHandle,
    map: MapHandle,
    styleUrl: String,
  ): String {
    repeat(10_000) {
      runtime.barrier()
      for (event in runtime.drainEvents().events) {
        if (
          event.type == RuntimeEventType.MAP_LOADING_FAILED &&
            event.mapSource == map &&
            event.message.contains(styleUrl)
        ) {
          return event.message
        }
      }
      runtime.barrier()
      waitForAsyncTestWork()
    }
    error("map loading failure for $styleUrl did not arrive")
  }

  private suspend fun waitForCondition(condition: suspend () -> Boolean): Boolean {
    repeat(10_000) {
      if (condition()) return true
      waitForAsyncTestWork()
    }
    return false
  }
}

private const val STYLE_JSON = """{"version":8,"sources":{},"layers":[]}"""
