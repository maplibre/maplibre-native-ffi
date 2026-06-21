package org.maplibre.nativeffi.internal.loader

import java.lang.foreign.MemorySegment
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.AbiVersionMismatchException
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.resource.ResourceErrorReason
import org.maplibre.nativeffi.resource.ResourceResponse
import org.maplibre.nativeffi.resource.ResourceResponseStatus
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

class NativeAccessTest {
  @Test
  fun abiVersionMismatchReportsActualAndExpectedVersions() {
    val error =
      assertFailsWith<AbiVersionMismatchException> {
        NativeAccess.checkAbiVersion(NativeAccess.EXPECTED_C_ABI_VERSION + 1)
      }

    assertEquals(NativeAccess.EXPECTED_C_ABI_VERSION + 1, error.actualVersion)
    assertEquals(NativeAccess.EXPECTED_C_ABI_VERSION, error.expectedVersion)
  }

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

  @Test
  fun resourceResponseRejectsEmbeddedNulWithBindingInvalidArgument() {
    NativeAccess.ensureLoaded()
    val response =
      ResourceResponse(ResourceResponseStatus.ERROR).apply {
        errorReason = ResourceErrorReason.OTHER
        errorMessage = "bad\u0000message"
      }

    val error =
      assertFailsWith<InvalidArgumentException> {
        NativeAccess.completeResourceRequest(MemorySegment.NULL, response)
      }

    assertEquals("error message contains embedded NUL", error.diagnostic)
  }

  @Test
  fun resourceResponseRejectsUnknownErrorReasonWithBindingInvalidArgument() {
    NativeAccess.ensureLoaded()
    val response =
      ResourceResponse(ResourceResponseStatus.ERROR).apply {
        errorReason = ResourceErrorReason(999)
      }

    val error =
      assertFailsWith<InvalidArgumentException> {
        NativeAccess.completeResourceRequest(MemorySegment.NULL, response)
      }

    assertEquals("Unknown resource error reason cannot be used as input: 999", error.diagnostic)
  }

  @Test
  fun runtimeOptionsRejectEmbeddedNulWithBindingInvalidArgument() {
    val error =
      assertFailsWith<InvalidArgumentException> {
        RuntimeHandle.create(
          RuntimeOptions().apply {
            assetPath = "bad\u0000path"
            cachePath = ":memory:"
          }
        )
      }

    assertEquals("C string inputs cannot contain embedded NUL characters", error.diagnostic)
  }
}
