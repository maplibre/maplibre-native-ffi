package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertSame
import kotlin.test.assertTrue
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.sizeOf
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.internal.c.mln_network_status_set
import org.maplibre.nativeffi.internal.c.mln_resource_transform_response
import org.maplibre.nativeffi.internal.c.mln_resource_transform_response_set_url
import org.maplibre.nativeffi.internal.memory.toCSize
import org.maplibre.nativeffi.internal.status.Status

@OptIn(ExperimentalForeignApi::class)
class NativeFrameAcquirePolicyTest : org.maplibre.nativeffi.NativeTestBase() {
  // BND-026, BND-167, BND-169, BND-172.

  @Test
  fun cleanupNativeFailureDoesNotReplaceOriginalNativeDiagnostic(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      memScoped {
        val failure =
          assertFailsWith<InvalidArgumentException> {
            Status.check(mln_network_status_set(999_999U))
          }
        val originalDiagnostic = failure.diagnostic
        val response = alloc<mln_resource_transform_response>()
        response.size = sizeOf<mln_resource_transform_response>().toUInt()
        val replacement = "https://example.com/style.json"

        val thrown =
          assertFailsWith<InvalidArgumentException> {
            FrameAcquirePolicy.cleanupAfterWrapperFailure(
              acquired = true,
              releaseNative = {
                Status.check(
                  mln_resource_transform_response_set_url(
                    response.ptr,
                    replacement,
                    replacement.length.toCSize(),
                  )
                )
              },
              closeLocal = {},
              failure = failure,
            )
          }

        assertSame(failure, thrown)
        assertEquals(originalDiagnostic, thrown.diagnostic)
        assertTrue(thrown.diagnostic.contains("network status"))
        assertTrue(Status.currentDiagnostic().contains("resource transform"))
      }
    }
}
