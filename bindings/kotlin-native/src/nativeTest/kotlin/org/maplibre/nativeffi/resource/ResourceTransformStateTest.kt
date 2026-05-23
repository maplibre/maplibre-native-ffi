package org.maplibre.nativeffi.resource

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNotEquals
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
  fun transformCallbackCopiesRequestAndKeepsReplacementUrlsIndependent() {
    var copiedRequest: ResourceTransformRequest? = null
    var index = 0
    val replacements =
      listOf("https://cdn.example.com/style.json", "https://cdn.example.com/style-2.json")
    val state =
      ResourceTransformState(
        ResourceTransformCallback { request ->
          copiedRequest = request
          replacements[index++]
        }
      )
    try {
      memScoped {
        val firstOut = alloc<mln_resource_transform_response>()
        val firstStatus =
          state.invoke(1U, "https://example.com/style.json".cstr.getPointer(this), firstOut.ptr)
        assertEquals(MaplibreStatus.OK.nativeCode, firstStatus)
        assertEquals(ResourceKind.STYLE, copiedRequest?.kind)
        assertEquals("https://example.com/style.json", copiedRequest?.url)
        assertEquals("https://cdn.example.com/style.json", firstOut.url?.toKString())

        val secondOut = alloc<mln_resource_transform_response>()
        val secondStatus =
          state.invoke(1U, "https://example.com/style-2.json".cstr.getPointer(this), secondOut.ptr)
        assertEquals(MaplibreStatus.OK.nativeCode, secondStatus)
        assertEquals("https://cdn.example.com/style.json", firstOut.url?.toKString())
        assertEquals("https://cdn.example.com/style-2.json", secondOut.url?.toKString())
        assertNotEquals(firstOut.url, secondOut.url)
      }
    } finally {
      state.close()
    }
  }

  @Test
  fun embeddedNulReplacementReturnsInvalidArgument() {
    val state =
      ResourceTransformState(
        ResourceTransformCallback { "https://cdn.example.com/\u0000truncated" }
      )
    try {
      memScoped {
        val out = alloc<mln_resource_transform_response>()
        assertEquals(MaplibreStatus.INVALID_ARGUMENT.nativeCode, state.invoke(1U, null, out.ptr))
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
