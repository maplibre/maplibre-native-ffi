package org.maplibre.nativeffi.internal.loader

import kotlin.test.Test
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue

class NativeAccessTest {
  @Test
  fun nativeAccessFailureIsWrappedWithJvmFlagGuidance() {
    val error =
      assertFailsWith<IllegalStateException> {
        NativeAccess.checkNativeAccessAndAbi { throw IllegalCallerException("native access") }
      }

    assertTrue(error.message.orEmpty().contains("--enable-native-access=ALL-UNNAMED"))
  }

  @Test
  fun missingSymbolFailureIsWrappedAsUnsatisfiedLinkError() {
    val error =
      assertFailsWith<UnsatisfiedLinkError> {
        NativeAccess.checkNativeAccessAndAbi { throw NoSuchElementException("mln_c_version") }
      }

    assertTrue(error.message.orEmpty().contains("Maplibre C ABI symbols"))
  }
}
