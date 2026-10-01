package org.maplibre.nativeffi.internal.loader

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertIs
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.error.AbiVersionMismatchException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.NativeErrorException

class AbiVersionTest {
  @Test
  fun abiVersionMismatchReportsActualAndExpectedVersions() {
    val error =
      assertFailsWith<AbiVersionMismatchException> {
        checkAbiVersion(Maplibre.EXPECTED_C_ABI_VERSION + 1)
      }

    assertEquals(Maplibre.EXPECTED_C_ABI_VERSION + 1, error.actualVersion)
    assertEquals(Maplibre.EXPECTED_C_ABI_VERSION, error.expectedVersion)
    assertIs<NativeErrorException>(error)
    assertEquals(MaplibreStatus.NATIVE_ERROR, error.status)
    assertEquals(MaplibreStatus.NATIVE_ERROR.nativeCode, error.nativeStatusCode)
    assertTrue(error.diagnostic.contains("expected ${Maplibre.EXPECTED_C_ABI_VERSION}"))
  }
}
