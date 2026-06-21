package org.maplibre.nativeffi.resource

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNull
import kotlin.test.assertTrue
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.cstr
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.c.mln_resource_transform_response
import org.maplibre.nativeffi.internal.callback.ResourceTransformState

@OptIn(ExperimentalForeignApi::class)
class ResourceTransformStateTest : org.maplibre.nativeffi.NativeTestBase() {
  @Test
  fun transformCallbackCopiesRequestAndPreservesUnknownKindRawValue() {
    var copiedRequest: ResourceTransformRequest? = null
    val state =
      ResourceTransformState(
        ResourceTransformCallback { request ->
          copiedRequest = request
          null
        }
      )
    try {
      memScoped {
        val out = alloc<mln_resource_transform_response>()
        val status =
          state.invoke(999U, "https://example.com/style.json".cstr.getPointer(this), out.ptr)
        assertEquals(MaplibreStatus.OK.nativeCode, status)
        assertNull(out.url)
      }
      assertEquals(ResourceKind(999), copiedRequest?.kind)
      assertEquals(999, copiedRequest?.kind?.nativeValue)
      assertEquals("https://example.com/style.json", copiedRequest?.url)
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

      // BND-121: host-language failures do not escape the C callback boundary.
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

  @Test
  fun closeDuringTransformCallbackCompletesAfterCallbackAndSuppressesLaterUpcalls() {
    var calls = 0
    lateinit var state: ResourceTransformState
    state =
      ResourceTransformState(
        ResourceTransformCallback {
          calls += 1
          state.close()
          assertFalse(state.isClosedForTesting())
          null
        }
      )
    memScoped {
      val out = alloc<mln_resource_transform_response>()

      assertEquals(MaplibreStatus.OK.nativeCode, state.invoke(1U, null, out.ptr))
      assertTrue(state.isClosedForTesting())
      assertEquals(MaplibreStatus.INVALID_ARGUMENT.nativeCode, state.invoke(1U, null, out.ptr))
      assertEquals(1, calls)
      state.close()
    }
  }
}
