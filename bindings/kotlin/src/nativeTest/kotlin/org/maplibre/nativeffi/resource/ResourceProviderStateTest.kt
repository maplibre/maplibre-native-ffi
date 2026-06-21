package org.maplibre.nativeffi.resource

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.StableRef
import kotlinx.cinterop.UByteVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.allocArray
import kotlinx.cinterop.asStableRef
import kotlinx.cinterop.cstr
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import kotlinx.cinterop.reinterpret
import kotlinx.cinterop.set
import kotlinx.cinterop.staticCFunction
import kotlinx.cinterop.value
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.c.mln_network_status_set
import org.maplibre.nativeffi.internal.c.mln_resource_request
import org.maplibre.nativeffi.internal.c.mln_runtime_destroy
import org.maplibre.nativeffi.internal.callback.ResourceProviderState
import org.maplibre.nativeffi.internal.struct.ResourceStructs
import platform.posix.pthread_create
import platform.posix.pthread_join
import platform.posix.pthread_tVar
import platform.posix.usleep

@OptIn(ExperimentalAtomicApi::class, ExperimentalForeignApi::class)
class ResourceProviderStateTest : org.maplibre.nativeffi.NativeTestBase() {
  // BND-141, BND-069: request and response values are copied before user retention.

  @Test
  fun providerCallbackCopiesRequestAndReturnsDecision() {
    var copied: ResourceRequest? = null
    val state =
      ResourceProviderState(
        ResourceProviderCallback { request, _ ->
          copied = request
          ResourceProviderDecision.PASS_THROUGH
        }
      )
    try {
      memScoped {
        val request = alloc<mln_resource_request>()
        request.url = "https://example.com/tile.pbf".cstr.getPointer(this)
        request.kind = 900U
        request.loading_method = 901U
        request.priority = 902U
        request.usage = 903U
        request.storage_policy = 904U
        request.has_range = true
        request.range_start = 7UL
        request.range_end = 11UL
        request.has_prior_modified = true
        request.prior_modified_unix_ms = 123L
        request.has_prior_expires = true
        request.prior_expires_unix_ms = 456L
        request.prior_etag = "etag".cstr.getPointer(this)
        val priorData = allocArray<UByteVar>(3)
        priorData[0] = 1U
        priorData[1] = 2U
        priorData[2] = 3U
        request.prior_data = priorData
        request.prior_data_size = 3UL
        val fakeHandle =
          alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_resource_request_handle>()
        assertEquals(
          ResourceProviderDecision.PASS_THROUGH.nativeValue.toUInt(),
          state.invoke(request.ptr, fakeHandle),
        )
      }
      assertEquals(ResourceKind(900), copied?.kind)
      assertEquals(900, copied?.kind?.nativeValue)
      assertEquals(ResourceLoadingMethod(901), copied?.loadingMethod)
      assertEquals(901, copied?.loadingMethod?.nativeValue)
      assertEquals(ResourcePriority(902), copied?.priority)
      assertEquals(902, copied?.priority?.nativeValue)
      assertEquals(ResourceUsage(903), copied?.usage)
      assertEquals(903, copied?.usage?.nativeValue)
      assertEquals(ResourceStoragePolicy(904), copied?.storagePolicy)
      assertEquals(904, copied?.storagePolicy?.nativeValue)
      assertEquals("https://example.com/tile.pbf", copied?.url)
      assertEquals(ResourceRequest.ByteRange(7, 11), copied?.range)
      assertEquals(123L, copied?.priorModifiedUnixMs)
      assertEquals(456L, copied?.priorExpiresUnixMs)
      assertEquals("etag", copied?.priorEtag)
      val firstPriorData = copied?.priorData ?: ByteArray(0)
      assertContentEquals(byteArrayOf(1, 2, 3), firstPriorData)
      firstPriorData[0] = 9
      assertContentEquals(byteArrayOf(1, 2, 3), copied?.priorData)
    } finally {
      state.close()
    }
  }

  @Test
  fun resourceResponseMaterializerSnapshotsOptionalFields() {
    memScoped {
      val response =
        ResourceResponse(ResourceResponseStatus.OK).apply {
          bytes = byteArrayOf(1, 2, 3)
          etag = "abc"
          modifiedUnixMs = 10L
          expiresUnixMs = 20L
          retryAfterUnixMs = 30L
          mustRevalidate = true
        }
      val native = ResourceStructs.resourceResponse(response, this).pointed
      assertEquals(ResourceResponseStatus.OK.nativeValue.toUInt(), native.status)
      assertEquals(3UL, native.byte_count)
      assertEquals(true, native.must_revalidate)
      assertEquals(true, native.has_modified)
      assertEquals(true, native.has_expires)
      assertEquals(true, native.has_retry_after)

      val bytes = response.bytes
      bytes[0] = 9
      assertContentEquals(byteArrayOf(1, 2, 3), response.bytes)
    }
  }

  @Test
  fun resourceResponseMaterializerRejectsEmbeddedNulWithBindingInvalidArgument() {
    memScoped {
      val response =
        ResourceResponse(ResourceResponseStatus.ERROR).apply {
          errorReason = ResourceErrorReason.OTHER
          etag = "bad\u0000etag"
        }

      val error =
        assertFailsWith<InvalidArgumentException> {
          ResourceStructs.resourceResponse(response, this)
        }

      assertEquals("ETag contains embedded NUL", error.diagnostic)
    }
  }

  @Test
  fun resourceResponseMaterializerRejectsUnknownErrorReasonWithBindingInvalidArgument() {
    memScoped {
      val response =
        ResourceResponse(ResourceResponseStatus.ERROR).apply {
          errorReason = ResourceErrorReason(999)
        }

      val error =
        assertFailsWith<InvalidArgumentException> {
          ResourceStructs.resourceResponse(response, this)
        }

      assertEquals("Unknown resource error reason cannot be used as input: 999", error.diagnostic)
    }
  }

  // BND-142, BND-150, BND-151: provider decision ownership and stale-handle behavior.

  @Test
  fun requestHandleReleasesProviderOwnedHandleOnceAndRejectsAfterClose() {
    memScoped {
      var releases = 0
      val fakeHandle =
        alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_resource_request_handle>()
      val handle = ResourceRequestHandle(fakeHandle, releaser = { releases++ })

      assertEquals(
        ResourceProviderDecision.HANDLE.nativeValue.toUInt(),
        handle.finishProviderDecision(ResourceProviderDecision.HANDLE),
      )
      handle.close()
      handle.close()
      assertEquals(1, releases)
      assertFailsWith<InvalidStateException> {
        handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
      }
    }
  }

  @Test
  fun providerOwnedHandleClosedBeforeDecisionReleasesAfterDecisionExactlyOnce() {
    memScoped {
      var releases = 0
      val fakeHandle =
        alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_resource_request_handle>()
      val handle = ResourceRequestHandle(fakeHandle, releaser = { releases++ })

      handle.close()
      assertEquals(
        ResourceProviderDecision.HANDLE.nativeValue.toUInt(),
        handle.finishProviderDecision(ResourceProviderDecision.HANDLE),
      )
      handle.close()
      assertEquals(1, releases)
    }
  }

  @Test
  fun passThroughDecisionLetsNativeOwnRelease() {
    memScoped {
      var releases = 0
      val fakeHandle =
        alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_resource_request_handle>()
      val handle = ResourceRequestHandle(fakeHandle, releaser = { releases++ })

      assertEquals(
        ResourceProviderDecision.PASS_THROUGH.nativeValue.toUInt(),
        handle.finishProviderDecision(ResourceProviderDecision.PASS_THROUGH),
      )
      handle.close()
      assertEquals(0, releases)
    }
  }

  @Test
  fun inlineCompletionBeforeProviderDecisionForcesHandledOwnership() {
    memScoped {
      var completions = 0
      var releases = 0
      val fakeHandle =
        alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_resource_request_handle>()
      val handle =
        ResourceRequestHandle(
          fakeHandle,
          completer = { _, _ ->
            completions++
            MaplibreStatus.OK.nativeCode
          },
          releaser = { releases++ },
        )

      handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
      assertEquals(
        ResourceProviderDecision.HANDLE.nativeValue.toUInt(),
        handle.finishProviderDecision(ResourceProviderDecision.PASS_THROUGH),
      )
      handle.close()

      assertEquals(1, completions)
      assertEquals(1, releases)
    }
  }

  @Test
  fun retainedPassThroughHandleCannotAffectLaterNativeRequest() {
    memScoped {
      var completions = 0
      var cancellationChecks = 0
      var releases = 0
      val fakeHandle =
        alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_resource_request_handle>()
      val handle =
        ResourceRequestHandle(
          fakeHandle,
          completer = { _, _ ->
            completions++
            MaplibreStatus.OK.nativeCode
          },
          cancellationChecker = { _, _ ->
            cancellationChecks++
            MaplibreStatus.OK.nativeCode
          },
          releaser = { releases++ },
        )

      assertEquals(
        ResourceProviderDecision.PASS_THROUGH.nativeValue.toUInt(),
        handle.finishProviderDecision(ResourceProviderDecision.PASS_THROUGH),
      )

      assertFailsWith<InvalidStateException> {
        handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
      }
      assertFailsWith<InvalidStateException> { handle.isCancelled() }
      handle.close()

      assertEquals(0, completions)
      assertEquals(0, cancellationChecks)
      assertEquals(0, releases)
    }
  }

  @Test
  fun successfulCompletionTwiceReportsAlreadyCompletedBeforeCrossingIntoNative() {
    memScoped {
      var completions = 0
      var releases = 0
      val fakeHandle =
        alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_resource_request_handle>()
      val handle =
        ResourceRequestHandle(
          fakeHandle,
          completer = { _, _ ->
            completions++
            MaplibreStatus.OK.nativeCode
          },
          releaser = { releases++ },
        )

      assertEquals(
        ResourceProviderDecision.HANDLE.nativeValue.toUInt(),
        handle.finishProviderDecision(ResourceProviderDecision.HANDLE),
      )
      handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
      val duplicate =
        assertFailsWith<InvalidStateException> {
          handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
        }

      assertEquals(MaplibreStatus.INVALID_STATE, duplicate.status)
      assertEquals(1, completions)
      assertEquals(1, releases)
    }
  }

  @Test
  fun releaseMakesLaterCompletionAndCancellationChecksFailBeforeNativeCalls() {
    memScoped {
      var completions = 0
      var cancellationChecks = 0
      var releases = 0
      val fakeHandle =
        alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_resource_request_handle>()
      val handle =
        ResourceRequestHandle(
          fakeHandle,
          completer = { _, _ ->
            completions++
            MaplibreStatus.OK.nativeCode
          },
          cancellationChecker = { _, _ ->
            cancellationChecks++
            MaplibreStatus.OK.nativeCode
          },
          releaser = { releases++ },
        )

      assertEquals(
        ResourceProviderDecision.HANDLE.nativeValue.toUInt(),
        handle.finishProviderDecision(ResourceProviderDecision.HANDLE),
      )
      handle.close()

      assertFailsWith<InvalidStateException> {
        handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
      }
      assertFailsWith<InvalidStateException> { handle.isCancelled() }
      handle.close()

      assertEquals(0, completions)
      assertEquals(0, cancellationChecks)
      assertEquals(1, releases)
    }
  }

  // BND-121, BND-123: host-language failure containment and close-during-callback
  // synchronization.

  @Test
  fun hostLanguageFailureTellsNativeNotToPassThrough() {
    val state =
      ResourceProviderState(
        ResourceProviderCallback { _, _ -> throw IllegalStateException("contained") }
      )
    try {
      memScoped {
        val request = alloc<mln_resource_request>()
        request.url = null
        val fakeHandle =
          alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_resource_request_handle>()
        assertEquals(UInt.MAX_VALUE, state.invoke(request.ptr, fakeHandle))
      }
    } finally {
      state.close()
    }
  }

  @Test
  fun closeDuringProviderCallbackCompletesAfterCallbackAndSuppressesLaterUpcalls() {
    memScoped {
      var calls = 0
      lateinit var state: ResourceProviderState
      state =
        ResourceProviderState(
          ResourceProviderCallback { _, _ ->
            calls += 1
            state.close()
            assertFalse(state.isClosedForTesting())
            ResourceProviderDecision.PASS_THROUGH
          }
        )
      val request = alloc<mln_resource_request>()
      request.url = null
      val fakeHandle =
        alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_resource_request_handle>()

      assertEquals(
        ResourceProviderDecision.PASS_THROUGH.nativeValue.toUInt(),
        state.invoke(request.ptr, fakeHandle),
      )
      assertTrue(state.isClosedForTesting())
      assertEquals(UInt.MAX_VALUE, state.invoke(request.ptr, fakeHandle))
      assertEquals(1, calls)
      state.close()
    }
  }

  // BND-146, BND-147, BND-148, BND-152, BND-153: handled request terminal states.

  @Test
  fun completionThatReachesNativeIsTerminalWhenNativeReturnsError() {
    memScoped {
      var completions = 0
      var releases = 0
      val fakeHandle =
        alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_resource_request_handle>()
      val handle =
        ResourceRequestHandle(
          fakeHandle,
          completer = { _, _ ->
            completions++
            MaplibreStatus.INVALID_STATE.nativeCode
          },
          releaser = { releases++ },
        )

      assertEquals(
        ResourceProviderDecision.HANDLE.nativeValue.toUInt(),
        handle.finishProviderDecision(ResourceProviderDecision.HANDLE),
      )
      val nativeFailure =
        assertFailsWith<InvalidStateException> {
          handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
        }
      assertEquals(MaplibreStatus.INVALID_STATE.nativeCode, nativeFailure.nativeStatusCode)
      assertEquals(1, completions)
      assertEquals(1, releases)

      assertFailsWith<InvalidStateException> {
        handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
      }
      assertEquals(1, completions)
      assertEquals(1, releases)
    }
  }

  @Test
  fun nativeCompletionFailureKeepsDiagnosticCapturedBeforeReleaseCleanup() {
    memScoped {
      val fakeHandle =
        alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_resource_request_handle>()
      val handle =
        ResourceRequestHandle(
          fakeHandle,
          completer = { _, _ -> mln_network_status_set(999_999U) },
          releaser = { mln_runtime_destroy(null) },
        )

      assertEquals(
        ResourceProviderDecision.HANDLE.nativeValue.toUInt(),
        handle.finishProviderDecision(ResourceProviderDecision.HANDLE),
      )
      val nativeFailure =
        assertFailsWith<InvalidArgumentException> {
          handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
        }

      assertTrue(nativeFailure.diagnostic.contains("network status"))
      assertFalse(nativeFailure.diagnostic.contains("runtime"))
    }
  }

  @Test
  fun closeDuringCompletionWaitsForCompletionBeforeNativeRelease() {
    memScoped {
      var releases = 0
      val fakeHandle =
        alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_resource_request_handle>()
      lateinit var handle: ResourceRequestHandle
      handle =
        ResourceRequestHandle(
          fakeHandle,
          completer = { _, _ ->
            handle.close()
            assertEquals(0, releases)
            MaplibreStatus.OK.nativeCode
          },
          releaser = { releases++ },
        )

      assertEquals(
        ResourceProviderDecision.HANDLE.nativeValue.toUInt(),
        handle.finishProviderDecision(ResourceProviderDecision.HANDLE),
      )
      handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))

      assertEquals(1, releases)
      handle.close()
      assertEquals(1, releases)
    }
  }

  @Test
  fun concurrentCloseDuringCompletionWaitsForCompletionBeforeNativeRelease() {
    memScoped {
      val phase = AtomicInt(RESOURCE_PHASE_READY)
      val error = AtomicReference<Throwable?>(null)
      val releases = AtomicInt(0)
      val fakeHandle =
        alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_resource_request_handle>()
      val handle =
        ResourceRequestHandle(
          fakeHandle,
          completer = { _, _ ->
            phase.store(RESOURCE_PHASE_ENTERED)
            waitForResourcePhase(phase, RESOURCE_PHASE_RELEASE)
            MaplibreStatus.OK.nativeCode
          },
          releaser = { releases.addAndFetch(1) },
        )

      assertEquals(
        ResourceProviderDecision.HANDLE.nativeValue.toUInt(),
        handle.finishProviderDecision(ResourceProviderDecision.HANDLE),
      )

      runResourceCompletionOnNativeThread(ConcurrentResourceCompletion(handle, phase, error)) {
        waitForResourcePhase(phase, RESOURCE_PHASE_ENTERED)
        handle.close()
        assertEquals(0, releases.load())
      }

      error.load()?.let { throw it }
      assertEquals(1, releases.load())
      assertFailsWith<InvalidStateException> {
        handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
      }
    }
  }

  @Test
  fun closeDuringCancellationCheckWaitsForCheckBeforeNativeRelease() {
    memScoped {
      var releases = 0
      val fakeHandle =
        alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_resource_request_handle>()
      lateinit var handle: ResourceRequestHandle
      handle =
        ResourceRequestHandle(
          fakeHandle,
          cancellationChecker = { _, outCancelled ->
            handle.close()
            assertEquals(0, releases)
            outCancelled.pointed.value = false
            MaplibreStatus.OK.nativeCode
          },
          releaser = { releases++ },
        )

      assertEquals(
        ResourceProviderDecision.HANDLE.nativeValue.toUInt(),
        handle.finishProviderDecision(ResourceProviderDecision.HANDLE),
      )
      assertEquals(false, handle.isCancelled())

      assertEquals(1, releases)
      handle.close()
      assertEquals(1, releases)
    }
  }
}

@OptIn(ExperimentalAtomicApi::class)
private class ConcurrentResourceCompletion(
  private val handle: ResourceRequestHandle,
  private val phase: AtomicInt,
  private val error: AtomicReference<Throwable?>,
) {
  fun run() {
    try {
      handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
    } catch (throwable: Throwable) {
      error.store(throwable)
    } finally {
      phase.store(RESOURCE_PHASE_FINISHED)
    }
  }

  fun release() {
    phase.store(RESOURCE_PHASE_RELEASE)
  }
}

@OptIn(ExperimentalForeignApi::class)
private fun runResourceCompletionOnNativeThread(
  completion: ConcurrentResourceCompletion,
  block: () -> Unit,
) {
  memScoped {
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
    try {
      block()
    } finally {
      completion.release()
      pthread_join(thread.ptr[0], null)
    }
  }
}

@OptIn(ExperimentalForeignApi::class)
private fun completeResourceRequestOnNativeThread(raw: COpaquePointer?): COpaquePointer? {
  val selfRef = requireNotNull(raw).asStableRef<ConcurrentResourceCompletion>()
  try {
    selfRef.get().run()
  } finally {
    selfRef.dispose()
  }
  return null
}

@OptIn(ExperimentalAtomicApi::class)
private fun waitForResourcePhase(phase: AtomicInt, expected: Int) {
  repeat(10_000) {
    if (phase.load() == expected) return
    usleep(1_000U)
  }
  error("timed out waiting for resource phase $expected")
}

private const val RESOURCE_PHASE_READY = 0
private const val RESOURCE_PHASE_ENTERED = 1
private const val RESOURCE_PHASE_RELEASE = 2
private const val RESOURCE_PHASE_FINISHED = 3
