package org.maplibre.nativeffi.resource

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNull
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.cstr
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.toKString
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.c.mln_resource_transform_response
import org.maplibre.nativeffi.internal.callback.ResourceTransformState

@OptIn(ExperimentalForeignApi::class)
class ResourceTransformStateTest {
  @Test
  fun transformCallbackCopiesRequestAndStoresReplacementUrl() {
    var copiedRequest: ResourceTransformRequest? = null
    val state =
      ResourceTransformState(
        ResourceTransformCallback { request ->
          copiedRequest = request
          "https://cdn.example.com/style.json"
        }
      )
    try {
      memScoped {
        val out = alloc<mln_resource_transform_response>()
        val status =
          state.invoke(1U, "https://example.com/style.json".cstr.getPointer(this), out.ptr)
        assertEquals(MaplibreStatus.OK.nativeCode, status)
        assertEquals(ResourceKind.STYLE, copiedRequest?.kind)
        assertEquals("https://example.com/style.json", copiedRequest?.url)
        assertEquals("https://cdn.example.com/style.json", out.url?.toKString())
      }
    } finally {
      state.close()
    }
  }

  @Test
  fun nullEmptyAndThrowingTransformsProduceNoReplacement() {
    memScoped {
      val nullState = ResourceTransformState(ResourceTransformCallback { null })
      val nullOut = alloc<mln_resource_transform_response>()
      assertEquals(MaplibreStatus.OK.nativeCode, nullState.invoke(3U, null, nullOut.ptr))
      assertNull(nullOut.url)
      nullState.close()

      val throwingState =
        ResourceTransformState(
          ResourceTransformCallback { throw IllegalStateException("contained") }
        )
      val throwingOut = alloc<mln_resource_transform_response>()
      assertEquals(
        MaplibreStatus.NATIVE_ERROR.nativeCode,
        throwingState.invoke(3U, null, throwingOut.ptr),
      )
      throwingState.close()
    }
  }
}
