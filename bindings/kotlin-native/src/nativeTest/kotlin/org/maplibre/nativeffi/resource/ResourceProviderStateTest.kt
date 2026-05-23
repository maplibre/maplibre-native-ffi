package org.maplibre.nativeffi.resource

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.cstr
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import kotlinx.cinterop.reinterpret
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
        request.kind = ResourceKind.TILE.nativeValue
        request.loading_method = ResourceLoadingMethod.NETWORK_ONLY.nativeValue
        request.priority = ResourcePriority.LOW.nativeValue
        request.usage = ResourceUsage.ONLINE.nativeValue
        request.storage_policy = ResourceStoragePolicy.VOLATILE.nativeValue
        val fakeHandle =
          alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_resource_request_handle>()
        assertEquals(
          ResourceProviderDecision.PASS_THROUGH.nativeValue,
          state.invoke(request.ptr, fakeHandle),
        )
      }
      assertEquals(ResourceKind.TILE, copied?.kind)
      assertEquals(ResourceLoadingMethod.NETWORK_ONLY, copied?.loadingMethod)
      assertEquals(ResourcePriority.LOW, copied?.priority)
      assertEquals(ResourceStoragePolicy.VOLATILE, copied?.storagePolicy)
      assertEquals("https://example.com/tile.pbf", copied?.url)
    } finally {
      state.close()
    }
  }

  @Test
  fun resourceResponseMaterializerCopiesOptionalFields() {
    memScoped {
      val response =
        ResourceResponse.ok(byteArrayOf(1, 2, 3))
          .etag("abc")
          .modifiedUnixMs(10L)
          .expiresUnixMs(20L)
          .retryAfterUnixMs(30L)
          .mustRevalidate(true)
      val native = ResourceStructs.resourceResponse(response, this).pointed
      assertEquals(ResourceResponseStatus.OK.nativeValue, native.status)
      assertEquals(3UL, native.byte_count)
      assertEquals(true, native.must_revalidate)
      assertEquals(true, native.has_modified)
      assertEquals(true, native.has_expires)
      assertEquals(true, native.has_retry_after)
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
        ResourceProviderDecision.HANDLE.nativeValue,
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
        ResourceProviderDecision.HANDLE.nativeValue,
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
        ResourceProviderDecision.PASS_THROUGH.nativeValue,
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
