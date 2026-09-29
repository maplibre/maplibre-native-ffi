package org.maplibre.nativeffi.internal.status

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.sizeOf
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.c.mln_network_status_set
import org.maplibre.nativeffi.internal.c.mln_resource_transform_response
import org.maplibre.nativeffi.internal.c.mln_resource_transform_response_set_url
import org.maplibre.nativeffi.internal.memory.MemoryUtil
import org.maplibre.nativeffi.internal.memory.toCSize

@OptIn(ExperimentalForeignApi::class)
class NativeStatusDiagnosticTest : org.maplibre.nativeffi.NativeTestBase() {
  @Test
  fun deterministicNativeStatusProducersThrowMappedExceptionTypes() {
    memScoped {
      val invalidArgument =
        assertFailsWith<InvalidArgumentException> {
          NativeDiagnostics.check { diagnostic -> mln_network_status_set(999_999U, diagnostic) }
        }
      assertEquals(MaplibreStatus.INVALID_ARGUMENT, invalidArgument.status)
      assertEquals(MaplibreStatus.INVALID_ARGUMENT.nativeCode, invalidArgument.nativeStatusCode)
      assertTrue(invalidArgument.diagnostic.contains("network status"))

      val response = alloc<mln_resource_transform_response>()
      response.size = sizeOf<mln_resource_transform_response>().toUInt()
      val replacement = "https://example.com/style.json"
      val invalidState =
        assertFailsWith<InvalidStateException> {
          NativeDiagnostics.check { diagnostic ->
            mln_resource_transform_response_set_url(
              response.ptr,
              replacement,
              replacement.length.toCSize(),
              diagnostic,
            )
          }
        }
      assertEquals(MaplibreStatus.INVALID_STATE, invalidState.status)
      assertTrue(invalidState.diagnostic.contains("resource transform"))
    }
  }

  @Test
  fun nullTerminatedStringsRejectEmbeddedNul() {
    memScoped {
      val error = assertFailsWith<InvalidArgumentException> { MemoryUtil.cString(this, "a\u0000b") }

      assertEquals(MaplibreStatus.INVALID_ARGUMENT, error.status)
      assertEquals(MaplibreStatus.INVALID_ARGUMENT.nativeCode, error.nativeStatusCode)
      assertEquals("C string inputs cannot contain embedded NUL characters", error.diagnostic)
    }
  }
}
