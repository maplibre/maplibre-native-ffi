package org.maplibre.nativeffi.internal.loader

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.AbiVersionMismatchException
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.RuntimeOptions
import org.maplibre.nativeffi.runtime.runSuspendTest

class NativeAccessTest {
  @Test
  fun abiVersionMismatchReportsActualAndExpectedVersions(): Unit = runSuspendTest {
    val error =
      assertFailsWith<AbiVersionMismatchException> {
        NativeAccess.checkAbiVersion(NativeAccess.EXPECTED_C_ABI_VERSION + 1)
      }

    assertEquals(NativeAccess.EXPECTED_C_ABI_VERSION + 1, error.actualVersion)
    assertEquals(NativeAccess.EXPECTED_C_ABI_VERSION, error.expectedVersion)
  }

  @Test
  fun nativeAccessFailureIsWrappedWithJvmFlagGuidance(): Unit = runSuspendTest {
    val error =
      assertFailsWith<IllegalStateException> {
        NativeAccess.checkNativeAccessAndAbi { throw IllegalCallerException("native access") }
      }

    assertTrue(error.message.orEmpty().contains("--enable-native-access=ALL-UNNAMED"))
  }

  @Test
  fun missingSymbolFailureIsWrappedAsUnsatisfiedLinkError(): Unit = runSuspendTest {
    val error =
      assertFailsWith<UnsatisfiedLinkError> {
        NativeAccess.checkNativeAccessAndAbi { throw NoSuchElementException("mln_c_version") }
      }

    assertTrue(error.message.orEmpty().contains("Maplibre C ABI symbols"))
  }

  @Test
  fun runtimeOptionsRejectEmbeddedNulWithBindingInvalidArgument(): Unit = runSuspendTest {
    val error =
      assertFailsWith<InvalidArgumentException> {
        GeneratedApi.runtimeCreate(
          RuntimeOptions(assetPath = "bad\u0000path", cachePath = ":memory:")
        )
      }

    assertEquals("text contains an embedded NUL", error.diagnostic)
  }
}
