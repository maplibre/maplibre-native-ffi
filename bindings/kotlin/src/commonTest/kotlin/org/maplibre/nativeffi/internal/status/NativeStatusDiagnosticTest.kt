package org.maplibre.nativeffi.internal.status

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.loader.ensureNativeLibrary
import org.maplibre.nativeffi.internal.memory.NativeArena
import org.maplibre.nativeffi.internal.memory.writeU32

class NativeStatusDiagnosticTest {
  @Test
  fun deterministicNativeStatusProducersThrowMappedExceptionTypes() {
    ensureNativeLibrary()
    NativeArena().use { arena ->
      val invalidArgument =
        assertFailsWith<InvalidArgumentException> {
          NativeDiagnostics.check { diagnostic -> C.mln_network_status_set(999_999, diagnostic) }
        }
      assertEquals(MaplibreStatus.INVALID_ARGUMENT, invalidArgument.status)
      assertEquals(MaplibreStatus.INVALID_ARGUMENT.nativeCode, invalidArgument.nativeStatusCode)
      assertTrue(invalidArgument.diagnostic.contains("network status"))

      // A response the C API never issued: its size is right, and its context is null.
      val response = arena.allocate(RESOURCE_TRANSFORM_RESPONSE_SIZE)
      writeU32(response, RESOURCE_TRANSFORM_RESPONSE_SIZE.toUInt())
      val replacement = "custom://replacement-style.json".encodeToByteArray()
      val invalidState =
        assertFailsWith<InvalidStateException> {
          NativeDiagnostics.check { diagnostic ->
            C.mln_resource_transform_response_set_url(
              response,
              arena.bytes(replacement),
              replacement.size.toLong(),
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
    NativeArena().use { arena ->
      val error = assertFailsWith<InvalidArgumentException> { arena.cString("a\u0000b") }

      assertEquals(MaplibreStatus.INVALID_ARGUMENT, error.status)
      assertEquals(MaplibreStatus.INVALID_ARGUMENT.nativeCode, error.nativeStatusCode)
      assertEquals("text contains an embedded NUL", error.diagnostic)
    }
  }

  private companion object {
    /** `sizeof(mln_resource_transform_response)`: a size, a URL pointer, and a context pointer. */
    val RESOURCE_TRANSFORM_RESPONSE_SIZE = org.maplibre.nativeffi.internal.memory.w(12, 24)
  }
}
