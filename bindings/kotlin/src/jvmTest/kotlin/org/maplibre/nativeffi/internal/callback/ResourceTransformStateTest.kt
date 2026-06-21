package org.maplibre.nativeffi.internal.callback

import java.lang.foreign.Arena
import java.lang.foreign.MemorySegment
import java.lang.foreign.ValueLayout
import kotlin.test.Test
import kotlin.test.assertEquals
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.resource.ResourceKind
import org.maplibre.nativeffi.resource.ResourceTransformCallback

class ResourceTransformStateTest {
  @Test
  fun transformCallbackCopiesRequestAndKeepsOriginalUrlWhenNullIsReturned() {
    var copiedKind: ResourceKind? = null
    var copiedUrl: String? = null
    ResourceTransformState(
        ResourceTransformCallback { request ->
          copiedKind = request.kind
          copiedUrl = request.url
          null
        }
      )
      .use { state ->
        Arena.ofConfined().use { arena ->
          val response = arena.allocate(24)
          val status =
            state.invoke(
              MemorySegment.NULL,
              ResourceKind.TILE.nativeValue,
              arena.allocateFrom("https://example.com/tile.pbf"),
              response,
            )

          assertEquals(MaplibreStatus.OK.nativeCode, status)
          assertEquals(ResourceKind.TILE, copiedKind)
          assertEquals("https://example.com/tile.pbf", copiedUrl)
          assertEquals(24, response.get(ValueLayout.JAVA_INT, 0))
          assertEquals(MemorySegment.NULL, response.get(ValueLayout.ADDRESS, 8))
        }
      }
  }

  @Test
  fun invalidRewriteUrlReturnsInvalidArgument() {
    ResourceTransformState(ResourceTransformCallback { "bad\u0000url" }).use { state ->
      Arena.ofConfined().use { arena ->
        val response = arena.allocate(24)
        val status =
          state.invoke(
            MemorySegment.NULL,
            ResourceKind.STYLE.nativeValue,
            arena.allocateFrom("https://example.com/style.json"),
            response,
          )

        assertEquals(MaplibreStatus.INVALID_ARGUMENT.nativeCode, status)
        assertEquals(24, response.get(ValueLayout.JAVA_INT, 0))
        assertEquals(MemorySegment.NULL, response.get(ValueLayout.ADDRESS, 8))
      }
    }
  }
}
