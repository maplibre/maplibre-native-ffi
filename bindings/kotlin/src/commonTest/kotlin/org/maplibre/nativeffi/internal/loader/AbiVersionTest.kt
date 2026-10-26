package org.maplibre.nativeffi.internal.loader

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import org.maplibre.nativeffi.AbiVersionMismatchException
import org.maplibre.nativeffi.EXPECTED_C_ABI_VERSION
import org.maplibre.nativeffi.Status

class AbiVersionTest {
  @Test
  fun abiVersionMismatchReportsActualAndExpectedVersions() {
    val error =
      assertFailsWith<AbiVersionMismatchException> {
        checkAbiVersion(EXPECTED_C_ABI_VERSION + 1)
      }

    assertEquals(EXPECTED_C_ABI_VERSION + 1, error.actualVersion)
    assertEquals(EXPECTED_C_ABI_VERSION, error.expectedVersion)
    assertEquals(Status.UNSUPPORTED, error.status)
    assertTrue(error.diagnostic.contains("expected ${EXPECTED_C_ABI_VERSION}"))
  }
}
