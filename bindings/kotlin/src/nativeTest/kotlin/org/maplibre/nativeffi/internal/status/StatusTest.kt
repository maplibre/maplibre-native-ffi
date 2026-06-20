package org.maplibre.nativeffi.internal.status

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
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
import org.maplibre.nativeffi.runtime.NetworkStatus

@OptIn(ExperimentalForeignApi::class)
class NativeStatusDiagnosticTest : org.maplibre.nativeffi.NativeTestBase() {
  // BND-020, BND-021, BND-022, BND-023, BND-024, BND-025, BND-026:
  // status mapping, diagnostic copies, and binding-owned diagnostics.

  @Test
  fun deterministicNativeStatusProducersThrowMappedExceptionTypes() {
    memScoped {
      val invalidArgument =
        assertFailsWith<InvalidArgumentException> { Status.check(mln_network_status_set(999_999U)) }
      assertEquals(MaplibreStatus.INVALID_ARGUMENT, invalidArgument.status)
      assertTrue(invalidArgument.diagnostic.contains("network status"))

      val response = alloc<mln_resource_transform_response>()
      response.size = sizeOf<mln_resource_transform_response>().toUInt()
      val replacement = "https://example.com/style.json"
      val invalidState =
        assertFailsWith<InvalidStateException> {
          Status.check(
            mln_resource_transform_response_set_url(
              response.ptr,
              replacement,
              replacement.length.toULong(),
            )
          )
        }
      assertEquals(MaplibreStatus.INVALID_STATE, invalidState.status)
      assertTrue(invalidState.diagnostic.contains("resource transform"))
    }
  }

  @Test
  fun thrownExceptionCarriesNativeStatusCodeAndCopiedDiagnostic() {
    val exception = assertFailsWith<InvalidArgumentException> { Status.check(-1) }

    assertEquals(MaplibreStatus.INVALID_ARGUMENT, exception.status)
    assertEquals(-1, exception.nativeStatusCode)
    assertEquals(Status.currentDiagnostic(), exception.diagnostic)
  }

  @Test
  fun nativeStatusConversionCapturesDiagnosticImmediately() {
    try {
      val exception =
        assertFailsWith<InvalidArgumentException> { Status.check(mln_network_status_set(999_999U)) }
      val diagnostic = exception.diagnostic

      assertEquals(MaplibreStatus.INVALID_ARGUMENT, exception.status)
      assertTrue(diagnostic.contains("network status"))

      Status.check(mln_network_status_set(NetworkStatus.ONLINE.nativeValue.toUInt()))

      assertEquals("", Status.currentDiagnostic())
      assertEquals(diagnostic, exception.diagnostic)
    } finally {
      Status.check(mln_network_status_set(NetworkStatus.ONLINE.nativeValue.toUInt()))
    }
  }

  @Test
  fun nullTerminatedStringsRejectEmbeddedNul() {
    memScoped {
      Status.exception(mln_network_status_set(999_999U))

      val error = assertFailsWith<InvalidArgumentException> { MemoryUtil.cString(this, "a\u0000b") }

      assertEquals(MaplibreStatus.INVALID_ARGUMENT, error.status)
      assertEquals(MaplibreStatus.INVALID_ARGUMENT.nativeCode, error.nativeStatusCode)
      assertEquals("C string inputs cannot contain embedded NUL characters", error.diagnostic)
      assertFalse(error.diagnostic.contains("network status"))
    }
  }
}
