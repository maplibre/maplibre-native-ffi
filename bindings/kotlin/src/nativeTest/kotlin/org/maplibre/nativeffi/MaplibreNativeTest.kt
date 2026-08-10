package org.maplibre.nativeffi

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertIs
import kotlin.test.assertTrue
import kotlinx.cinterop.ExperimentalForeignApi
import org.maplibre.nativeffi.error.AbiVersionMismatchException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.NativeErrorException

@OptIn(ExperimentalForeignApi::class)
class MaplibreNativeTest : org.maplibre.nativeffi.NativeTestBase() {
  @Test
  fun abiVersionMismatchUsesStableBindingError() {
    val error =
      assertFailsWith<AbiVersionMismatchException> {
        Maplibre.checkCompatibleCAbi(Maplibre.EXPECTED_C_ABI_VERSION + 1L)
      }

    assertEquals(MaplibreStatus.NATIVE_ERROR, error.status)
    assertIs<NativeErrorException>(error)
    assertEquals(MaplibreStatus.NATIVE_ERROR.nativeCode, error.nativeStatusCode)
    assertEquals(Maplibre.EXPECTED_C_ABI_VERSION + 1L, error.actualVersion)
    assertEquals(Maplibre.EXPECTED_C_ABI_VERSION, error.expectedVersion)
    assertTrue(error.diagnostic.contains("C ABI version"))
    assertTrue(error.diagnostic.contains("expected ${Maplibre.EXPECTED_C_ABI_VERSION}"))
  }
}
