package org.maplibre.nativeffi.runtime

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
import kotlinx.cinterop.asStableRef
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.reinterpret
import kotlinx.cinterop.set
import kotlinx.cinterop.sizeOf
import kotlinx.cinterop.staticCFunction
import kotlinx.cinterop.toCValues
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.error.AbiVersionMismatchException
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.NativeErrorException
import org.maplibre.nativeffi.error.WrongThreadException
import org.maplibre.nativeffi.internal.c.mln_runtime_event
import org.maplibre.nativeffi.internal.c.mln_runtime_event_payload
import org.maplibre.nativeffi.internal.callback.ResourceProviderState
import org.maplibre.nativeffi.internal.callback.ResourceTransformState
import org.maplibre.nativeffi.internal.lifecycle.SyntheticHandles
import org.maplibre.nativeffi.internal.lifecycle.rawHandleValue
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions
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
class RuntimeHandleNativeTest : org.maplibre.nativeffi.NativeTestBase() {
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

  @Test
  fun resourceProviderCompletesHandledRequestFromAnotherNativeThread() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val handledRequest = AtomicReference<ResourceRequestHandle?>(null)
    val completionError = AtomicReference<Throwable?>(null)
    try {
      runtime.setResourceProvider(
        ResourceProviderCallback { request, handle ->
          if (request.requestedUrl != "custom://threaded-style.json") {
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

  // BND-044, BND-046.

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

      runtime.pump(0)

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

      runtime.pump(0)
    } finally {
      runtime.close()
    }
    assertTrue(runtime.isClosed)
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
    map.close()
    try {
      var copiedEvent: RuntimeEvent? = null
      val syntheticSource = SyntheticHandles.map().rawHandleValue
      memScoped {
        val event = alloc<mln_runtime_event>()
        event.type = RuntimeEventType.MAP_STYLE_LOADED.nativeValue.toUInt()
        event.source_type = RuntimeEventSourceType.MAP.nativeValue.toUInt()
        event.source = syntheticSource
        event.code = 0
        event.payload_type = 0U
        event.message_offset = 0U
        event.message_size = 0U

        copiedEvent = runtime.copyEventForTesting(event, null)
      }
      val event = assertNotNull(copiedEvent)
      assertEquals(RuntimeEventType.MAP_STYLE_LOADED, event.type)
      assertEquals(RuntimeEventSourceType.MAP, event.sourceType)
      assertNull(event.mapSource)
      assertNull(event.runtimeSource)
      // The identity the native record carried survives a source that resolves to
      // no live wrapper.
      assertEquals(syntheticSource.toLong(), event.sourceId)
      assertEquals(RuntimeEventPayload.None, event.payload)
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
        // The message arena holds unrelated bytes ahead of this event's message,
        // so a decode that ignored message_offset would read the wrong text.
        val messageBytes = "padfuture event".encodeToByteArray()
        val messages = messageBytes.toCValues().getPointer(this)
        val event = alloc<mln_runtime_event>()
        event.type = 900U
        event.source_type = 901U
        event.source = 0x5Au
        event.code = 902
        event.payload_type = 903U
        event.message_offset = 3U
        event.message_size = (messageBytes.size - 3).toUInt()
        val payload = event.payload.ptr.reinterpret<ByteVar>()
        for (index in 0 until payloadWindowSize) {
          payload[index] = 0
        }
        payload[0] = 1
        payload[1] = 2
        payload[2] = 3

        copied = runtime.copyEventForTesting(event, messages)
        payload[0] = 9
      }

      val event = assertNotNull(copied)
      assertEquals(RuntimeEventType(900), event.type)
      assertEquals(900, event.type.nativeValue)
      assertEquals(RuntimeEventSourceType(901), event.sourceType)
      assertEquals(901, event.sourceType.nativeValue)
      assertNull(event.runtimeSource)
      assertNull(event.mapSource)
      // An unnamed source kind still reports the identity the record carried.
      assertEquals(0x5AL, event.sourceId)
      assertEquals(902, event.code)
      assertEquals("future event", event.message)
      val payload = event.payload as RuntimeEventPayload.Unknown
      assertEquals(903, payload.rawPayloadType)
      // The window is the whole inline union, and the copy survives the mutation
      // above.
      assertEquals(payloadWindowSize, payload.payloadBytes.size)
      assertContentEquals(
        byteArrayOf(1, 2, 3) + ByteArray(payloadWindowSize - 3),
        payload.payloadBytes,
      )
    } finally {
      runtime.close()
    }
  }

  // BND-084.

  private fun waitForMapEvent(
    runtime: RuntimeHandle,
    map: MapHandle,
    eventType: RuntimeEventType,
  ): Boolean {
    repeat(10_000) {
      runtime.pump(0)
      for (event in runtime.drainEvents().events) {
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
      runtime.pump(0)
      runtime
        .drainEvents()
        .events
        .find { it.type == eventType && it.mapSource == map }
        ?.let {
          return it
        }
      usleep(1_000U)
    }
    error("runtime event $eventType did not arrive")
  }

  /**
   * Loads a style URL whose scheme no file source serves; the failure names the scheme and URL,
   * proving the request reached the network file source.
   */
  private fun waitForHandledRequest(
    runtime: RuntimeHandle,
    handledRequest: AtomicReference<ResourceRequestHandle?>,
  ): ResourceRequestHandle {
    repeat(10_000) {
      handledRequest.load()?.let {
        return it
      }
      runtime.pump(0)
      usleep(1_000U)
    }
    error("resource provider did not receive handled request")
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
      runtime.pump(0)
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

/** Bytes of the inline payload union that one event record carries. */
@OptIn(ExperimentalForeignApi::class)
private val payloadWindowSize: Int = sizeOf<mln_runtime_event_payload>().toInt()

private object RuntimeHandleTestStyle {
  const val styleJson: String = "{\"version\":8,\"sources\":{},\"layers\":[]}"
}
