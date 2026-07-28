package org.maplibre.nativeffi.runtime

import java.util.concurrent.atomic.AtomicInteger
import java.util.concurrent.atomic.AtomicReference
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState
import org.maplibre.nativeffi.resource.ResourceKind
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceProviderDecision
import org.maplibre.nativeffi.resource.ResourceResponse
import org.maplibre.nativeffi.resource.ResourceResponseStatus
import org.maplibre.nativeffi.resource.ResourceTransformCallback

class RuntimeHandleTest {
  @Test
  fun runtimeRunsOnceAndCloses() {
    val runtime = RuntimeHandle.create(RuntimeOptions())

    assertFalse(runtime.isClosed)
    runtime.pump(0)
    runtime.close()
    runtime.close()

    assertTrue(runtime.isClosed)
    assertFailsWith<InvalidStateException> { runtime.pump(0) }
  }

  @Test
  fun freshRuntimeHasNoQueuedEvent() {
    RuntimeHandle.create(RuntimeOptions()).use { runtime -> assertNull(runtime.pollEvent()) }
  }

  @Test
  fun ambientCacheOperationRetainsRuntimeUntilDiscarded() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val operation = runtime.startAmbientCacheOperation(AmbientCacheOperation.INVALIDATE)

    assertFalse(operation.isClosed)
    assertFailsWith<InvalidStateException> { runtime.close() }

    operation.close()
    operation.close()

    assertTrue(operation.isClosed)
    runtime.close()
    assertTrue(runtime.isClosed)
  }

  @Test
  fun offlineDownloadStateUnknownRawValueRejectsBeforeNativeCall() {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      assertFailsWith<InvalidArgumentException> {
        runtime.startSetOfflineRegionDownloadState(1, OfflineRegionDownloadState(900))
      }
    }
  }

  @Test
  fun geometryOfflineRegionDefinitionStartsOperation() {
    RuntimeHandle.create(RuntimeOptions().apply { cachePath = ":memory:" }).use { runtime ->
      val operation =
        runtime.startCreateOfflineRegion(
          OfflineRegionDefinition.GeometryRegion(
            "custom://style.json",
            Geometry.Point(LatLng(1.0, 2.0)),
            0.0,
            1.0,
            1.0f,
            false,
          ),
          ByteArray(0),
        )
      assertEquals(OfflineOperationKind.REGION_CREATE, operation.kind)
      assertEquals(OfflineOperationResultKind.REGION, operation.resultKind)
      operation.close()
      assertTrue(operation.isClosed)
    }
  }

  @Test
  fun offlineRegionsListCompletesAndConsumesOperation() {
    RuntimeHandle.create(RuntimeOptions().apply { cachePath = ":memory:" }).use { runtime ->
      val operation = runtime.startOfflineRegions()

      val completed = waitForOperation(runtime, operation)
      assertEquals(OfflineOperationKind.REGIONS_LIST, completed.operationKind)
      assertEquals(OfflineOperationResultKind.REGION_LIST, completed.resultKind)

      assertTrue(runtime.takeOfflineRegionsResult(operation).isEmpty())
      assertTrue(operation.isClosed)
      assertFailsWith<InvalidStateException> { runtime.takeOfflineRegionsResult(operation) }
    }
  }

  @Test
  fun resourceProviderCompletesStyleRequestThroughRuntime() {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      val calls = AtomicInteger(0)
      val callbackError = AtomicReference<Throwable?>(null)
      runtime.setResourceProvider(
        ResourceProviderCallback { request, handle ->
          try {
            if (request.url != "custom://jvm-style.json") {
              return@ResourceProviderCallback ResourceProviderDecision.PASS_THROUGH
            }
            calls.incrementAndGet()
            assertEquals(ResourceKind.STYLE, request.kind)
            handle.complete(
              ResourceResponse(ResourceResponseStatus.OK).apply {
                bytes = STYLE_JSON.encodeToByteArray()
              }
            )
            ResourceProviderDecision.HANDLE
          } catch (error: Throwable) {
            callbackError.set(error)
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
        map.setStyleUrl("custom://jvm-style.json")
        assertTrue(waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED))
        callbackError.get()?.let { throw AssertionError("resource provider callback failed", it) }
        assertEquals(1, calls.get())
      } finally {
        map.close()
      }
    }
  }

  // BND-154: an installed provider is consulted, a replacement takes over while a
  // map is live, and a cleared provider leaves requests to the network file source.
  @Test
  fun resourceProviderIsConsultedUntilClearedWhileMapIsLive() {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      val firstCalls = AtomicInteger(0)
      val secondCalls = AtomicInteger(0)
      runtime.setResourceProvider(
        ResourceProviderCallback { _, _ ->
          firstCalls.incrementAndGet()
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
        assertTrue(firstCalls.get() > 0)

        // Replacing the provider while a map is live is part of the C API contract.
        runtime.setResourceProvider(
          ResourceProviderCallback { _, _ ->
            secondCalls.incrementAndGet()
            ResourceProviderDecision.PASS_THROUGH
          }
        )
        val firstCallsAfterReplace = firstCalls.get()
        loadUnservedStyle(runtime, map, "jar:file:/packaged/second.json")
        assertTrue(secondCalls.get() > 0)
        assertEquals(firstCallsAfterReplace, firstCalls.get())

        runtime.clearResourceProvider()
        val secondCallsAfterClear = secondCalls.get()
        loadUnservedStyle(runtime, map, "jar:file:/packaged/third.json")
        assertEquals(firstCallsAfterReplace, firstCalls.get())
        assertEquals(secondCallsAfterClear, secondCalls.get())

        // Clearing an already cleared provider stays a successful no-op.
        runtime.clearResourceProvider()
      } finally {
        map.close()
      }
    }
  }

  @Test
  fun runtimeCloseDuringResourceProviderCallbackRejectsBeforeNativeDestroy() {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      val closeError = AtomicReference<Throwable?>(null)
      runtime.setResourceProvider(
        ResourceProviderCallback { request, _ ->
          if (request.url == "custom://close-during-provider.json") {
            closeError.set(assertFailsWith<InvalidStateException> { runtime.close() })
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
            runtime.pump(0)
            closeError.get() != null
          }
        )
        assertFalse(runtime.isClosed)
      } finally {
        map.close()
      }
    }
  }

  @Test
  fun runtimeCloseDuringResourceTransformCallbackRejectsBeforeNativeDestroy() {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      val closeError = AtomicReference<Throwable?>(null)
      runtime.setResourceTransform(
        ResourceTransformCallback { request ->
          if (request.url == "http://example.invalid/close-during-transform.json") {
            closeError.set(assertFailsWith<InvalidStateException> { runtime.close() })
          }
          null
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
            runtime.pump(0)
            closeError.get() != null
          }
        )
        assertFalse(runtime.isClosed)
      } finally {
        map.close()
      }
    }
  }

  private fun waitForOperation(
    runtime: RuntimeHandle,
    operation: OfflineOperationHandle<*>,
  ): RuntimeEventPayload.OfflineOperationCompleted {
    repeat(10_000) {
      runtime.pump(0)
      while (true) {
        val event = runtime.pollEvent() ?: break
        val completed = event.payload as? RuntimeEventPayload.OfflineOperationCompleted ?: continue
        if (completed.operationId != operation.id) continue
        assertEquals(RuntimeEventType.OFFLINE_OPERATION_COMPLETED, event.type)
        assertEquals(operation.kind, completed.operationKind)
        assertEquals(operation.resultKind, completed.resultKind)
        assertEquals(MaplibreStatus.OK.nativeCode, completed.resultStatus)
        return completed
      }
      Thread.sleep(1)
    }
    error("offline operation did not complete: ${operation.id}")
  }

  private fun waitForMapEvent(
    runtime: RuntimeHandle,
    map: MapHandle,
    type: RuntimeEventType,
  ): Boolean {
    repeat(10_000) {
      runtime.pump(0)
      while (true) {
        val event = runtime.pollEvent() ?: break
        if (event.type == type && event.mapSource == map) return true
      }
      Thread.sleep(1)
    }
    return false
  }

  /**
   * Loads a style URL whose scheme no file source serves, so the loading failure that names the
   * scheme and the URL proves the request reached the network file source.
   */
  private fun loadUnservedStyle(runtime: RuntimeHandle, map: MapHandle, styleUrl: String) {
    map.setStyleUrl(styleUrl)
    val message = waitForMapLoadingFailure(runtime, map, styleUrl)
    assertTrue(message.contains("\"jar\""), "unexpected loading failure message: $message")
  }

  private fun waitForMapLoadingFailure(
    runtime: RuntimeHandle,
    map: MapHandle,
    styleUrl: String,
  ): String {
    repeat(10_000) {
      runtime.pump(0)
      while (true) {
        val event = runtime.pollEvent() ?: break
        if (
          event.type == RuntimeEventType.MAP_LOADING_FAILED &&
            event.mapSource == map &&
            event.message.contains(styleUrl)
        ) {
          return event.message
        }
      }
      Thread.sleep(1)
    }
    error("map loading failure for $styleUrl did not arrive")
  }

  private fun waitForCondition(condition: () -> Boolean): Boolean {
    repeat(10_000) {
      if (condition()) return true
      Thread.sleep(1)
    }
    return false
  }
}

private const val STYLE_JSON = """{"version":8,"sources":{},"layers":[]}"""
