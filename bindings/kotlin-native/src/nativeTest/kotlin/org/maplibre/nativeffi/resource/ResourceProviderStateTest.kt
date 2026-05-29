package org.maplibre.nativeffi.resource

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.UByteVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.allocArray
import kotlinx.cinterop.cstr
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import kotlinx.cinterop.reinterpret
import kotlinx.cinterop.set
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.internal.c.mln_resource_request
import org.maplibre.nativeffi.internal.callback.ResourceProviderState
import org.maplibre.nativeffi.internal.struct.ResourceStructs

@OptIn(ExperimentalForeignApi::class)
class ResourceProviderStateTest {
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
        request.kind = ResourceKind.TILE.nativeValue.toUInt()
        request.loading_method = ResourceLoadingMethod.NETWORK_ONLY.nativeValue.toUInt()
        request.priority = ResourcePriority.LOW.nativeValue.toUInt()
        request.usage = ResourceUsage.OFFLINE.nativeValue.toUInt()
        request.storage_policy = ResourceStoragePolicy.VOLATILE.nativeValue.toUInt()
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
      assertEquals(ResourceKind.TILE, copied?.kind)
      assertEquals(ResourceKind.TILE.nativeValue, copied?.rawKind)
      assertEquals(ResourceLoadingMethod.NETWORK_ONLY, copied?.loadingMethod)
      assertEquals(ResourceLoadingMethod.NETWORK_ONLY.nativeValue, copied?.rawLoadingMethod)
      assertEquals(ResourcePriority.LOW, copied?.priority)
      assertEquals(ResourcePriority.LOW.nativeValue, copied?.rawPriority)
      assertEquals(ResourceUsage.OFFLINE, copied?.usage)
      assertEquals(ResourceUsage.OFFLINE.nativeValue, copied?.rawUsage)
      assertEquals(ResourceStoragePolicy.VOLATILE, copied?.storagePolicy)
      assertEquals(ResourceStoragePolicy.VOLATILE.nativeValue, copied?.rawStoragePolicy)
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
  fun resourceResponseMaterializerCopiesOptionalFields() {
    memScoped {
      val response =
        ResourceResponse.ok(byteArrayOf(1, 2, 3)).apply {
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
  fun requestHandleReleasesProviderOwnedHandleOnceAndRejectsAfterClose() {
    memScoped {
      var releases = 0
      val fakeHandle =
        alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_resource_request_handle>()
      val handle = ResourceRequestHandle(fakeHandle) { releases++ }

      assertEquals(
        ResourceProviderDecision.HANDLE.nativeValue.toUInt(),
        handle.finishProviderDecision(ResourceProviderDecision.HANDLE),
      )
      handle.close()
      handle.close()
      assertEquals(1, releases)
      assertFailsWith<InvalidStateException> { handle.complete(ResourceResponse.noContent()) }
    }
  }

  @Test
  fun providerOwnedHandleClosedBeforeDecisionReleasesAfterDecisionExactlyOnce() {
    memScoped {
      var releases = 0
      val fakeHandle =
        alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_resource_request_handle>()
      val handle = ResourceRequestHandle(fakeHandle) { releases++ }

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
      val handle = ResourceRequestHandle(fakeHandle) { releases++ }

      assertEquals(
        ResourceProviderDecision.PASS_THROUGH.nativeValue.toUInt(),
        handle.finishProviderDecision(ResourceProviderDecision.PASS_THROUGH),
      )
      handle.close()
      assertEquals(0, releases)
    }
  }

  @Test
  fun exceptionDecisionTellsNativeNotToPassThrough() {
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
}
