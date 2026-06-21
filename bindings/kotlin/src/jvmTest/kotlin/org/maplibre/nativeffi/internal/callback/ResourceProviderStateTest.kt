package org.maplibre.nativeffi.internal.callback

import java.lang.foreign.Arena
import java.lang.foreign.MemorySegment
import java.lang.foreign.ValueLayout
import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import org.maplibre.nativeffi.resource.ResourceKind
import org.maplibre.nativeffi.resource.ResourceLoadingMethod
import org.maplibre.nativeffi.resource.ResourcePriority
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceProviderDecision
import org.maplibre.nativeffi.resource.ResourceRequest
import org.maplibre.nativeffi.resource.ResourceStoragePolicy
import org.maplibre.nativeffi.resource.ResourceUsage

class ResourceProviderStateTest {
  @Test
  fun providerCallbackCopiesRequestAndReturnsDecision() {
    var copied: ResourceRequest? = null
    ResourceProviderState(
        ResourceProviderCallback { request, _ ->
          copied = request
          ResourceProviderDecision.PASS_THROUGH
        }
      )
      .use { state ->
        Arena.ofConfined().use { arena ->
          val request = resourceRequest(arena)
          val fakeHandle = arena.allocate(8)

          assertEquals(
            ResourceProviderDecision.PASS_THROUGH.nativeValue,
            state.invoke(MemorySegment.NULL, request, fakeHandle),
          )
        }
      }

    assertEquals(ResourceKind(900), copied?.kind)
    assertEquals(ResourceLoadingMethod(901), copied?.loadingMethod)
    assertEquals(ResourcePriority(902), copied?.priority)
    assertEquals(ResourceUsage(903), copied?.usage)
    assertEquals(ResourceStoragePolicy(904), copied?.storagePolicy)
    assertEquals("https://example.com/tile.pbf", copied?.url)
    assertEquals(ResourceRequest.ByteRange(7, 11), copied?.range)
    assertEquals(123L, copied?.priorModifiedUnixMs)
    assertEquals(456L, copied?.priorExpiresUnixMs)
    assertEquals("etag", copied?.priorEtag)
    val priorData = copied?.priorData ?: ByteArray(0)
    assertContentEquals(byteArrayOf(1, 2, 3), priorData)
    priorData[0] = 9
    assertContentEquals(byteArrayOf(1, 2, 3), copied?.priorData)
  }

  @Test
  fun hostLanguageFailureTellsNativeNotToPassThrough() {
    ResourceProviderState(
        ResourceProviderCallback { _, _ -> throw IllegalStateException("contained") }
      )
      .use { state ->
        Arena.ofConfined().use { arena ->
          assertEquals(
            UNKNOWN_DECISION,
            state.invoke(MemorySegment.NULL, resourceRequest(arena), arena.allocate(8)),
          )
        }
      }
  }

  private fun resourceRequest(arena: Arena): MemorySegment {
    val request = arena.allocate(RESOURCE_REQUEST_SIZE)
    val priorData = arena.allocate(3)
    priorData.set(ValueLayout.JAVA_BYTE, 0, 1)
    priorData.set(ValueLayout.JAVA_BYTE, 1, 2)
    priorData.set(ValueLayout.JAVA_BYTE, 2, 3)

    request.set(
      ValueLayout.ADDRESS,
      RESOURCE_REQUEST_URL_OFFSET,
      arena.allocateFrom("https://example.com/tile.pbf"),
    )
    request.set(ValueLayout.JAVA_INT, RESOURCE_REQUEST_KIND_OFFSET, 900)
    request.set(ValueLayout.JAVA_INT, RESOURCE_REQUEST_LOADING_METHOD_OFFSET, 901)
    request.set(ValueLayout.JAVA_INT, RESOURCE_REQUEST_PRIORITY_OFFSET, 902)
    request.set(ValueLayout.JAVA_INT, RESOURCE_REQUEST_USAGE_OFFSET, 903)
    request.set(ValueLayout.JAVA_INT, RESOURCE_REQUEST_STORAGE_POLICY_OFFSET, 904)
    request.set(ValueLayout.JAVA_BOOLEAN, RESOURCE_REQUEST_HAS_RANGE_OFFSET, true)
    request.set(ValueLayout.JAVA_LONG, RESOURCE_REQUEST_RANGE_START_OFFSET, 7L)
    request.set(ValueLayout.JAVA_LONG, RESOURCE_REQUEST_RANGE_END_OFFSET, 11L)
    request.set(ValueLayout.JAVA_BOOLEAN, RESOURCE_REQUEST_HAS_PRIOR_MODIFIED_OFFSET, true)
    request.set(ValueLayout.JAVA_LONG, RESOURCE_REQUEST_PRIOR_MODIFIED_OFFSET, 123L)
    request.set(ValueLayout.JAVA_BOOLEAN, RESOURCE_REQUEST_HAS_PRIOR_EXPIRES_OFFSET, true)
    request.set(ValueLayout.JAVA_LONG, RESOURCE_REQUEST_PRIOR_EXPIRES_OFFSET, 456L)
    request.set(ValueLayout.ADDRESS, RESOURCE_REQUEST_PRIOR_ETAG_OFFSET, arena.allocateFrom("etag"))
    request.set(ValueLayout.ADDRESS, RESOURCE_REQUEST_PRIOR_DATA_OFFSET, priorData)
    request.set(ValueLayout.JAVA_LONG, RESOURCE_REQUEST_PRIOR_DATA_SIZE_OFFSET, 3L)
    return request
  }

  private companion object {
    private const val UNKNOWN_DECISION: Int = -1

    private const val RESOURCE_REQUEST_SIZE: Long = 112
    private const val RESOURCE_REQUEST_URL_OFFSET: Long = 8
    private const val RESOURCE_REQUEST_KIND_OFFSET: Long = 16
    private const val RESOURCE_REQUEST_LOADING_METHOD_OFFSET: Long = 20
    private const val RESOURCE_REQUEST_PRIORITY_OFFSET: Long = 24
    private const val RESOURCE_REQUEST_USAGE_OFFSET: Long = 28
    private const val RESOURCE_REQUEST_STORAGE_POLICY_OFFSET: Long = 32
    private const val RESOURCE_REQUEST_HAS_RANGE_OFFSET: Long = 36
    private const val RESOURCE_REQUEST_RANGE_START_OFFSET: Long = 40
    private const val RESOURCE_REQUEST_RANGE_END_OFFSET: Long = 48
    private const val RESOURCE_REQUEST_HAS_PRIOR_MODIFIED_OFFSET: Long = 56
    private const val RESOURCE_REQUEST_PRIOR_MODIFIED_OFFSET: Long = 64
    private const val RESOURCE_REQUEST_HAS_PRIOR_EXPIRES_OFFSET: Long = 72
    private const val RESOURCE_REQUEST_PRIOR_EXPIRES_OFFSET: Long = 80
    private const val RESOURCE_REQUEST_PRIOR_ETAG_OFFSET: Long = 88
    private const val RESOURCE_REQUEST_PRIOR_DATA_OFFSET: Long = 96
    private const val RESOURCE_REQUEST_PRIOR_DATA_SIZE_OFFSET: Long = 104
  }
}
