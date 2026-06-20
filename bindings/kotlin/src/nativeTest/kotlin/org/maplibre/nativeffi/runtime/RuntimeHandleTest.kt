package org.maplibre.nativeffi.runtime

import cnames.structs.mln_runtime
import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertIs
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertSame
import kotlin.test.assertTrue
import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.StableRef
import kotlinx.cinterop.alloc
import kotlinx.cinterop.allocArray
import kotlinx.cinterop.asStableRef
import kotlinx.cinterop.cstr
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.reinterpret
import kotlinx.cinterop.set
import kotlinx.cinterop.sizeOf
import kotlinx.cinterop.staticCFunction
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.error.AbiVersionMismatchException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.NativeErrorException
import org.maplibre.nativeffi.error.WrongThreadException
import org.maplibre.nativeffi.internal.c.mln_runtime_event
import org.maplibre.nativeffi.internal.callback.ResourceProviderState
import org.maplibre.nativeffi.internal.callback.ResourceTransformState
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.offline.OfflineRegionInfo
import org.maplibre.nativeffi.resource.ResourceErrorReason
import org.maplibre.nativeffi.resource.ResourceKind
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceProviderDecision
import org.maplibre.nativeffi.resource.ResourceRequestHandle
import org.maplibre.nativeffi.resource.ResourceResponse
import org.maplibre.nativeffi.resource.ResourceResponseStatus
import org.maplibre.nativeffi.resource.ResourceTransformCallback
import platform.posix.pthread_create
import platform.posix.pthread_join
import platform.posix.pthread_tVar
import platform.posix.usleep

@OptIn(ExperimentalAtomicApi::class, ExperimentalForeignApi::class)
class RuntimeHandleTest : org.maplibre.nativeffi.NativeTestBase() {
  // BND-001, BND-040, BND-041: runtime identity, release, and ABI guard behavior.

  @Test
  fun closeReleasesRuntimeOnceAndInvalidatesWrapper() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())

    assertFalse(runtime.isClosed)
    runtime.runOnce()
    runtime.pollEvent()
    runtime.close()

    assertTrue(runtime.isClosed)
    runtime.close()
    assertFailsWith<InvalidStateException> { runtime.runOnce() }
  }

  @Test
  fun closeCallsRuntimeDestroyExactlyOnceAndInvalidatesWrapper() {
    memScoped {
      var destroys = 0
      val runtime =
        RuntimeHandle(alloc<ByteVar>().ptr.reinterpret<mln_runtime>()) {
          destroys += 1
          MaplibreStatus.OK.nativeCode
        }

      assertFalse(runtime.isClosed)
      runtime.close()
      runtime.close()

      assertEquals(1, destroys)
      assertTrue(runtime.isClosed)
      assertFailsWith<InvalidStateException> { runtime.runOnce() }
    }
  }

  @Test
  fun abiMismatchPreventsNativeRuntimeCreation() {
    var creates = 0

    val error =
      assertFailsWith<AbiVersionMismatchException> {
        RuntimeHandle.createForTesting(
          actualAbiVersion = Maplibre.EXPECTED_C_ABI_VERSION + 1L,
          creator = { _, _ ->
            creates += 1
            MaplibreStatus.OK.nativeCode
          },
        )
      }

    assertEquals(MaplibreStatus.NATIVE_ERROR, error.status)
    assertIs<NativeErrorException>(error)
    assertEquals(Maplibre.EXPECTED_C_ABI_VERSION + 1L, error.actualVersion)
    assertEquals(Maplibre.EXPECTED_C_ABI_VERSION, error.expectedVersion)
    assertEquals(0, creates)
  }

  // BND-122: callback replacement retains existing state and releases failed replacement state.

  @Test
  fun resourceProviderAndTransformReplacementPathsRetainAndClearCallbackState() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    try {
      runtime.setResourceTransform(ResourceTransformCallback { request -> request.url })
      runtime.setResourceTransform(ResourceTransformCallback { null })
      runtime.clearResourceTransform()
      runtime.clearResourceTransform()

      runtime.setResourceProvider(
        ResourceProviderCallback { _, _ -> ResourceProviderDecision.PASS_THROUGH }
      )
      runtime.setResourceProvider(
        ResourceProviderCallback { _, _ -> ResourceProviderDecision.PASS_THROUGH }
      )
    } finally {
      runtime.close()
    }
  }

  @Test
  fun failedResourceCallbackReplacementPreservesPreviousAndClosesReplacement() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    try {
      runtime.setResourceTransform(ResourceTransformCallback { request -> request.url })
      val initialTransform = runtime.resourceTransformStateForTesting()
      var failedTransform: ResourceTransformState? = null

      val transformError =
        assertFailsWith<MaplibreException> {
          runtime.setResourceTransformForTesting(ResourceTransformCallback { null }) { replacement
            ->
            failedTransform = replacement
            MaplibreStatus.NATIVE_ERROR.nativeCode
          }
        }

      assertEquals(MaplibreStatus.NATIVE_ERROR, transformError.status)
      assertSame(initialTransform, runtime.resourceTransformStateForTesting())
      assertFalse(initialTransform?.isClosedForTesting() ?: true)
      assertTrue(failedTransform?.isClosedForTesting() == true)

      runtime.setResourceProvider(
        ResourceProviderCallback { _, _ -> ResourceProviderDecision.PASS_THROUGH }
      )
      val initialProvider = runtime.resourceProviderStateForTesting()
      var failedProvider: ResourceProviderState? = null

      val providerError =
        assertFailsWith<MaplibreException> {
          runtime.setResourceProviderForTesting(
            ResourceProviderCallback { _, _ -> ResourceProviderDecision.PASS_THROUGH }
          ) { replacement ->
            failedProvider = replacement
            MaplibreStatus.NATIVE_ERROR.nativeCode
          }
        }

      assertEquals(MaplibreStatus.NATIVE_ERROR, providerError.status)
      assertSame(initialProvider, runtime.resourceProviderStateForTesting())
      assertFalse(initialProvider?.isClosedForTesting() ?: true)
      assertTrue(failedProvider?.isClosedForTesting() == true)
    } finally {
      runtime.close()
    }
  }

  // BND-101, BND-140, BND-141, BND-142, BND-143, BND-144, BND-145, BND-146,
  // BND-147, BND-148, BND-149: style loading with resource transform/provider behavior.

  @Test
  fun resourceTransformRewritesNetworkStyleUrlAndCanBeClearedAfterMapCreation() {
    val previousNetworkStatus = Maplibre.networkStatus
    Maplibre.setNetworkStatus(NetworkStatus.ONLINE)
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val transformCalls = AtomicInt(0)
    val lastTransformUrl = AtomicReference<String?>(null)
    val lastTransformKind = AtomicReference<ResourceKind?>(null)
    try {
      runtime.setResourceTransform(
        ResourceTransformCallback { request ->
          transformCalls.addAndFetch(1)
          lastTransformUrl.store(request.url)
          lastTransformKind.store(request.kind)
          "unsupported://rewritten-style.json"
        }
      )

      val map =
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 128
            height = 128
          },
        )
      try {
        val initialTransform = runtime.resourceTransformStateForTesting()
        map.setStyleUrl("http://example.invalid/original-style.json")
        assertTrue(waitForTransformCall(runtime, transformCalls))
        assertEquals(1, transformCalls.load())
        assertEquals("http://example.invalid/original-style.json", lastTransformUrl.load())
        assertEquals(ResourceKind.STYLE, lastTransformKind.load())

        runtime.clearResourceTransform()
        assertNull(runtime.resourceTransformStateForTesting())
        assertTrue(initialTransform?.isClosedForTesting() == true)

        map.setStyleUrl("http://example.invalid/after-clear-style.json")
        assertNoAdditionalTransformCalls(runtime, transformCalls, 1)
      } finally {
        map.close()
      }
    } finally {
      runtime.close()
      Maplibre.setNetworkStatus(previousNetworkStatus)
    }
  }

  @Test
  fun resourceProviderPassThroughDelegatesToNativeLoadingAndClosesRequestHandle() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val passThroughRequest = AtomicReference<ResourceRequestHandle?>(null)
    val calls = AtomicInt(0)
    try {
      runtime.setResourceProvider(
        ResourceProviderCallback { request, handle ->
          if (request.url == "custom://pass-through-style.json") {
            calls.addAndFetch(1)
            passThroughRequest.store(handle)
          }
          ResourceProviderDecision.PASS_THROUGH
        }
      )
      val map =
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 128
            height = 128
          },
        )
      try {
        map.setStyleUrl("custom://pass-through-style.json")
        val handle = waitForHandledRequest(runtime, passThroughRequest)

        assertFailsWith<InvalidStateException> { handle.isCancelled() }
        assertFailsWith<InvalidStateException> {
          handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
        }

        val failure = waitForMapEventRecord(runtime, map, RuntimeEventType.MAP_LOADING_FAILED)
        assertEquals(RuntimeEventType.MAP_LOADING_FAILED, failure.type)
        assertEquals(map, failure.mapSource)
        assertEquals(1, calls.load())
      } finally {
        map.close()
      }
    } finally {
      runtime.close()
    }
  }

  @Test
  fun resourceProviderCompletesStyleRequestInlineThroughPublicApi() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val calls = AtomicInt(0)
    val callbackError = AtomicReference<Throwable?>(null)
    try {
      runtime.setResourceProvider(
        ResourceProviderCallback { request, handle ->
          try {
            if (request.url != "custom://style.json") {
              return@ResourceProviderCallback ResourceProviderDecision.PASS_THROUGH
            }
            calls.addAndFetch(1)
            assertEquals(ResourceKind.STYLE, request.kind)
            handle.complete(
              ResourceResponse(ResourceResponseStatus.OK).apply {
                bytes = STYLE_JSON.encodeToByteArray()
              }
            )
            assertFailsWith<InvalidStateException> {
              handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
            }
            assertFailsWith<InvalidStateException> { handle.isCancelled() }
            ResourceProviderDecision.PASS_THROUGH
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
            width = 128
            height = 128
          },
        )
      try {
        map.setStyleUrl("custom://style.json")
        val styleLoaded = waitForMapEventRecord(runtime, map, RuntimeEventType.MAP_STYLE_LOADED)
        val copiedMessage = styleLoaded.message
        assertEquals(RuntimeEventType.MAP_STYLE_LOADED, styleLoaded.type)
        assertEquals(RuntimeEventSourceType.MAP, styleLoaded.sourceType)
        assertEquals(map, styleLoaded.mapSource)
        assertNull(styleLoaded.runtimeSource)
        assertEquals(RuntimeEventPayload.None, styleLoaded.payload)
        runtime.pollEvent()
        assertEquals(copiedMessage, styleLoaded.message)
        assertEquals(1, calls.load())
        assertNull(callbackError.load())
      } finally {
        map.close()
      }
    } finally {
      runtime.close()
    }
  }

  @Test
  fun resourceProviderCompletesHandledRequestAfterCallbackReturns() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val handledRequest = AtomicReference<ResourceRequestHandle?>(null)
    try {
      runtime.setResourceProvider(
        ResourceProviderCallback { request, handle ->
          if (request.url != "custom://async-style.json") {
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
            width = 128
            height = 128
          },
        )
      try {
        map.setStyleUrl("custom://async-style.json")
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
        handle.close()
        assertTrue(waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED))
      } finally {
        map.close()
      }
    } finally {
      runtime.close()
    }
  }

  @Test
  fun resourceProviderCompletesHandledRequestFromAnotherNativeThread() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val handledRequest = AtomicReference<ResourceRequestHandle?>(null)
    val completionError = AtomicReference<Throwable?>(null)
    try {
      runtime.setResourceProvider(
        ResourceProviderCallback { request, handle ->
          if (request.url != "custom://threaded-style.json") {
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
            width = 128
            height = 128
          },
        )
      try {
        map.setStyleUrl("custom://threaded-style.json")
        val handle = waitForHandledRequest(runtime, handledRequest)
        completeOnNativeThread(handle, completionError)
        completionError.load()?.let { throw it }
        assertTrue(waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED))
      } finally {
        map.close()
      }
    } finally {
      runtime.close()
    }
  }

  // BND-044, BND-046: owner-thread calls report copied diagnostics and leave handles retryable.

  @Test
  fun runtimeOwnerThreadCallFromAnotherNativeThreadReportsCopiedDiagnostic() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val callError = AtomicReference<Throwable?>(null)
    try {
      runRuntimeOnNativeThread(runtime, callError)
      val error = callError.load()
      if (error !is WrongThreadException)
        throw error ?: AssertionError("wrong-thread call succeeded")
      val diagnostic = error.diagnostic
      assertEquals(MaplibreStatus.WRONG_THREAD, error.status)
      assertEquals(MaplibreStatus.WRONG_THREAD.nativeCode, error.nativeStatusCode)
      assertTrue(diagnostic.isNotBlank())

      runtime.runOnce()

      assertEquals(diagnostic, error.diagnostic)
    } finally {
      runtime.close()
    }
  }

  @Test
  fun runtimeCloseFromAnotherNativeThreadReportsWrongThreadAndLeavesHandleLive() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val closeError = AtomicReference<Throwable?>(null)
    try {
      runRuntimeCloseOnNativeThread(runtime, closeError)
      val error = closeError.load()
      if (error !is WrongThreadException)
        throw error ?: AssertionError("wrong-thread close succeeded")
      assertEquals(MaplibreStatus.WRONG_THREAD, error.status)
      assertFalse(runtime.isClosed)

      runtime.runOnce()
    } finally {
      runtime.close()
    }
    assertTrue(runtime.isClosed)
  }

  @Test
  fun resourceProviderErrorResponseBecomesCopiedMapLoadingFailureEvent() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    try {
      runtime.setResourceProvider(
        ResourceProviderCallback { request, handle ->
          if (request.url != "custom://error-style.json") {
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
            width = 128
            height = 128
          },
        )
      try {
        map.setStyleUrl("custom://error-style.json")
        val failure = waitForMapEventRecord(runtime, map, RuntimeEventType.MAP_LOADING_FAILED)
        val copiedMessage = failure.message
        assertEquals(RuntimeEventType.MAP_LOADING_FAILED, failure.type)
        assertEquals(RuntimeEventSourceType.MAP, failure.sourceType)
        assertEquals(map, failure.mapSource)
        assertNull(failure.runtimeSource)
        assertEquals(RuntimeEventPayload.None, failure.payload)
        assertTrue(copiedMessage.contains("custom style failed"))
        runtime.pollEvent()
        assertEquals(copiedMessage, failure.message)
      } finally {
        map.close()
      }
    } finally {
      runtime.close()
    }
  }

  @Test
  fun resourceProviderObservesCancellationBeforeLateCompletion() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val handledRequest = AtomicReference<ResourceRequestHandle?>(null)
    try {
      runtime.setResourceProvider(
        ResourceProviderCallback { request, handle ->
          if (request.url != "custom://cancelled-style.json") {
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
            width = 128
            height = 128
          },
        )
      map.setStyleUrl("custom://cancelled-style.json")
      val handle = waitForHandledRequest(runtime, handledRequest)

      map.close()

      assertTrue(waitForRequestCancellation(runtime, handle))
      val lateCompletion =
        assertFailsWith<InvalidStateException> {
          handle.complete(
            ResourceResponse(ResourceResponseStatus.OK).apply {
              bytes = STYLE_JSON.encodeToByteArray()
            }
          )
        }
      assertEquals(MaplibreStatus.INVALID_STATE, lateCompletion.status)
      assertTrue(lateCompletion.diagnostic.isNotBlank())
      handle.close()
    } finally {
      runtime.close()
    }
  }

  // BND-080, BND-081, BND-082, BND-083, BND-085, BND-086, BND-087: runtime
  // event copy, polling, stale-source, and unknown-domain behavior.

  @Test
  fun runOnceProcessesStyleLoadedEventAndPollingDrainsQueue() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 128
          height = 128
        },
      )
    try {
      map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}")

      var styleLoaded: RuntimeEvent? = null
      var reachedEmptyQueue = false
      repeat(20) {
        runtime.runOnce()
        while (true) {
          val event = runtime.pollEvent()
          if (event == null) {
            reachedEmptyQueue = true
            break
          }
          if (event.type == RuntimeEventType.MAP_STYLE_LOADED) {
            styleLoaded = event
          }
        }
      }

      val event = assertNotNull(styleLoaded)
      val copiedMessage = event.message
      assertTrue(reachedEmptyQueue)
      assertEquals(RuntimeEventSourceType.MAP, event.sourceType)
      assertEquals(map, event.mapSource)
      assertNull(event.runtimeSource)
      assertEquals(RuntimeEventPayload.None, event.payload)
      runtime.pollEvent()
      assertEquals(copiedMessage, event.message)
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun mapOriginatedEventWithoutLiveWrapperExposesNoMapHandle() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 128
          height = 128
        },
      )
    val address = map.nativeAddress()
    map.close()
    try {
      var copiedEvent: RuntimeEvent? = null
      memScoped {
        val staleSource = alloc<ByteVar>()
        val event = alloc<mln_runtime_event>()
        event.size = sizeOf<mln_runtime_event>().toUInt()
        event.type = RuntimeEventType.MAP_STYLE_LOADED.nativeValue.toUInt()
        event.source_type = RuntimeEventSourceType.MAP.nativeValue.toUInt()
        event.source = staleSource.ptr
        event.code = 0
        event.payload_type = 0U
        event.payload = null
        event.payload_size = 0UL
        event.message = null
        event.message_size = 0UL

        copiedEvent = runtime.copyEventForTesting(event)
      }
      val event = assertNotNull(copiedEvent)
      assertEquals(RuntimeEventType.MAP_STYLE_LOADED, event.type)
      assertEquals(RuntimeEventSourceType.MAP, event.sourceType)
      assertNull(event.mapSource)
      assertNull(event.runtimeSource)
      assertEquals(RuntimeEventPayload.None, event.payload)

      assertNull(
        runtime.applyEventSideEffectsForTesting(
          RuntimeEventType.MAP_STYLE_LOADED,
          RuntimeEventSourceType.MAP,
          address,
        )
      )
      assertNull(
        runtime.applyEventSideEffectsForTesting(
          RuntimeEventType.MAP_STYLE_LOADED,
          RuntimeEventSourceType.MAP,
          address + 4096,
        )
      )
      assertNull(
        runtime.applyEventSideEffectsForTesting(
          RuntimeEventType.MAP_STYLE_LOADED,
          RuntimeEventSourceType.RUNTIME,
          address,
        )
      )
    } finally {
      runtime.close()
    }
  }

  @Test
  fun unknownRuntimeEventDomainsPreserveRawValuesAndCopiedPayload() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    var copied: RuntimeEvent? = null
    try {
      memScoped {
        val payload = allocArray<ByteVar>(3)
        payload[0] = 1
        payload[1] = 2
        payload[2] = 3
        val message = "future event"
        val event = alloc<mln_runtime_event>()
        event.size = sizeOf<mln_runtime_event>().toUInt()
        event.type = 900U
        event.source_type = 901U
        event.source = null
        event.code = 902
        event.payload_type = 903U
        event.payload = payload
        event.payload_size = 3UL
        event.message = message.cstr.getPointer(this)
        event.message_size = message.encodeToByteArray().size.toULong()

        copied = runtime.copyEventForTesting(event)
        payload[0] = 9
      }

      val event = assertNotNull(copied)
      assertEquals(RuntimeEventType(900), event.type)
      assertEquals(900, event.type.nativeValue)
      assertEquals(RuntimeEventSourceType(901), event.sourceType)
      assertEquals(901, event.sourceType.nativeValue)
      assertNull(event.runtimeSource)
      assertNull(event.mapSource)
      assertEquals(902, event.code)
      assertEquals("future event", event.message)
      val payload = event.payload as RuntimeEventPayload.Unknown
      assertEquals(903, payload.rawPayloadType)
      assertEquals(3L, payload.payloadSize)
      assertContentEquals(byteArrayOf(1, 2, 3), payload.payloadBytes)
    } finally {
      runtime.close()
    }
  }

  // BND-084: offline operation token validation, retryability, and consumption behavior.

  @Test
  fun offlineOperationTakeMethodsValidateExpectedOperationKindBeforeNativeCall() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val operation =
      OfflineOperationHandle<OfflineRegionInfo>(
        runtime,
        1UL,
        OfflineOperationKind.AMBIENT_CACHE,
        OfflineOperationResultKind.NONE,
      )
    try {
      assertFailsWith<InvalidStateException> { runtime.takeCreateOfflineRegionResult(operation) }
      assertFalse(operation.isClosed)
    } finally {
      operation.markConsumed()
      runtime.close()
    }
  }

  @Test
  fun failedOfflineOperationTakeLeavesHandleRetryable() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val operation =
      OfflineOperationHandle<OfflineRegionInfo>(
        runtime,
        1UL,
        OfflineOperationKind.REGION_CREATE,
        OfflineOperationResultKind.REGION,
      )
    try {
      assertFailsWith<MaplibreException> { runtime.takeCreateOfflineRegionResult(operation) }
      assertFalse(operation.isClosed)
      assertFailsWith<MaplibreException> { runtime.takeCreateOfflineRegionResult(operation) }
      assertFalse(operation.isClosed)
    } finally {
      operation.markConsumed()
      runtime.close()
    }
  }

  @Test
  fun consumedOfflineOperationCloseIsNoOp() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val operation =
      OfflineOperationHandle<Unit>(
        runtime,
        1UL,
        OfflineOperationKind.AMBIENT_CACHE,
        OfflineOperationResultKind.NONE,
      )
    try {
      operation.markConsumed()
      operation.close()
      operation.close()
      assertTrue(operation.isClosed)
    } finally {
      runtime.close()
    }
  }

  @Test
  fun runtimeCloseFailsWhileOfflineOperationIsLive() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val operation =
      OfflineOperationHandle<Unit>(
        runtime,
        1UL,
        OfflineOperationKind.AMBIENT_CACHE,
        OfflineOperationResultKind.NONE,
      )
    try {
      val error = assertFailsWith<InvalidStateException> { runtime.close() }
      assertEquals(MaplibreStatus.INVALID_STATE, error.status)
      assertEquals("RuntimeHandle has 1 live child handle(s)", error.diagnostic)
      assertFalse(operation.isClosed)
    } finally {
      operation.markConsumed()
      runtime.close()
    }
  }

  private fun waitForMapEvent(
    runtime: RuntimeHandle,
    map: MapHandle,
    eventType: RuntimeEventType,
  ): Boolean {
    repeat(10_000) {
      runtime.runOnce()
      while (true) {
        val event = runtime.pollEvent() ?: break
        if (event.type == eventType && event.mapSource == map) return true
        if (event.type == RuntimeEventType.MAP_LOADING_FAILED) {
          throw MaplibreException.forStatus(
            MaplibreStatus.NATIVE_ERROR,
            MaplibreStatus.NATIVE_ERROR.nativeCode,
            event.message,
          )
        }
      }
      usleep(1_000U)
    }
    return false
  }

  private fun waitForMapEventRecord(
    runtime: RuntimeHandle,
    map: MapHandle,
    eventType: RuntimeEventType,
  ): RuntimeEvent {
    repeat(10_000) {
      runtime.runOnce()
      while (true) {
        val event = runtime.pollEvent() ?: break
        if (event.type == eventType && event.mapSource == map) return event
      }
      usleep(1_000U)
    }
    error("runtime event $eventType did not arrive")
  }

  private fun waitForTransformCall(runtime: RuntimeHandle, calls: AtomicInt): Boolean {
    repeat(10_000) {
      runtime.runOnce()
      while (runtime.pollEvent() != null) {
        // Drain events so native loading can keep advancing.
      }
      if (calls.load() > 0) return true
      usleep(1_000U)
    }
    return false
  }

  private fun assertNoAdditionalTransformCalls(
    runtime: RuntimeHandle,
    calls: AtomicInt,
    expected: Int,
  ) {
    repeat(100) {
      runtime.runOnce()
      while (runtime.pollEvent() != null) {
        // Drain events so native loading can keep advancing.
      }
      assertEquals(expected, calls.load())
      usleep(1_000U)
    }
  }

  private fun waitForHandledRequest(
    runtime: RuntimeHandle,
    handledRequest: AtomicReference<ResourceRequestHandle?>,
  ): ResourceRequestHandle {
    repeat(10_000) {
      handledRequest.load()?.let {
        return it
      }
      runtime.runOnce()
      usleep(1_000U)
    }
    error("resource provider did not receive handled request")
  }

  private fun waitForRequestCancellation(
    runtime: RuntimeHandle,
    handle: ResourceRequestHandle,
  ): Boolean {
    repeat(10_000) {
      if (handle.isCancelled()) return true
      runtime.runOnce()
      usleep(1_000U)
    }
    return false
  }

  private fun completeOnNativeThread(
    handle: ResourceRequestHandle,
    completionError: AtomicReference<Throwable?>,
  ) {
    memScoped {
      val completion = BackgroundResourceCompletion(handle, completionError)
      val selfRef = StableRef.create(completion)
      val thread = alloc<pthread_tVar>()
      val status =
        pthread_create(
          thread.ptr,
          null,
          staticCFunction(::completeResourceRequestOnNativeThread),
          selfRef.asCPointer(),
        )
      if (status != 0) {
        selfRef.dispose()
        error("pthread_create failed with status $status")
      }
      pthread_join(thread.ptr[0], null)
    }
  }

  private fun runRuntimeOnNativeThread(
    runtime: RuntimeHandle,
    callError: AtomicReference<Throwable?>,
  ) {
    memScoped {
      val call = BackgroundRuntimeCall(runtime, callError)
      val selfRef = StableRef.create(call)
      val thread = alloc<pthread_tVar>()
      val status =
        pthread_create(
          thread.ptr,
          null,
          staticCFunction(::runRuntimeCallOnNativeThread),
          selfRef.asCPointer(),
        )
      if (status != 0) {
        selfRef.dispose()
        error("pthread_create failed with status $status")
      }
      pthread_join(thread.ptr[0], null)
    }
  }

  private fun runRuntimeCloseOnNativeThread(
    runtime: RuntimeHandle,
    closeError: AtomicReference<Throwable?>,
  ) {
    memScoped {
      val close = BackgroundRuntimeClose(runtime, closeError)
      val selfRef = StableRef.create(close)
      val thread = alloc<pthread_tVar>()
      val status =
        pthread_create(
          thread.ptr,
          null,
          staticCFunction(::closeRuntimeOnNativeThread),
          selfRef.asCPointer(),
        )
      if (status != 0) {
        selfRef.dispose()
        error("pthread_create failed with status $status")
      }
      pthread_join(thread.ptr[0], null)
    }
  }

  private companion object {
    private const val STYLE_JSON = "{\"version\":8,\"sources\":{},\"layers\":[]}"
  }
}

@OptIn(ExperimentalAtomicApi::class)
private class BackgroundResourceCompletion(
  private val handle: ResourceRequestHandle,
  private val error: AtomicReference<Throwable?>,
) {
  fun run() {
    try {
      handle.complete(
        ResourceResponse(ResourceResponseStatus.OK).apply {
          bytes = RuntimeHandleTestStyle.styleJson.encodeToByteArray()
        }
      )
    } catch (throwable: Throwable) {
      error.store(throwable)
    }
  }
}

@OptIn(ExperimentalForeignApi::class)
private fun completeResourceRequestOnNativeThread(raw: COpaquePointer?): COpaquePointer? {
  val selfRef = requireNotNull(raw).asStableRef<BackgroundResourceCompletion>()
  try {
    selfRef.get().run()
  } finally {
    selfRef.dispose()
  }
  return null
}

@OptIn(ExperimentalAtomicApi::class)
private class BackgroundRuntimeCall(
  private val runtime: RuntimeHandle,
  private val error: AtomicReference<Throwable?>,
) {
  fun run() {
    try {
      runtime.runOnce()
    } catch (throwable: Throwable) {
      error.store(throwable)
    }
  }
}

@OptIn(ExperimentalForeignApi::class)
private fun runRuntimeCallOnNativeThread(raw: COpaquePointer?): COpaquePointer? {
  val selfRef = requireNotNull(raw).asStableRef<BackgroundRuntimeCall>()
  try {
    selfRef.get().run()
  } finally {
    selfRef.dispose()
  }
  return null
}

@OptIn(ExperimentalAtomicApi::class)
private class BackgroundRuntimeClose(
  private val runtime: RuntimeHandle,
  private val error: AtomicReference<Throwable?>,
) {
  fun run() {
    try {
      runtime.close()
    } catch (throwable: Throwable) {
      error.store(throwable)
    }
  }
}

@OptIn(ExperimentalForeignApi::class)
private fun closeRuntimeOnNativeThread(raw: COpaquePointer?): COpaquePointer? {
  val selfRef = requireNotNull(raw).asStableRef<BackgroundRuntimeClose>()
  try {
    selfRef.get().run()
  } finally {
    selfRef.dispose()
  }
  return null
}

private object RuntimeHandleTestStyle {
  const val styleJson: String = "{\"version\":8,\"sources\":{},\"layers\":[]}"
}
