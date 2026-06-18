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
import org.maplibre.nativeffi.internal.status.Status

@OptIn(ExperimentalForeignApi::class)
class FrameAcquirePolicyTest {
  // BND-026, BND-167, BND-169, BND-172: frame-acquire cleanup preserves original failures.

  @Test
  fun wrapperFailureAfterNativeAcquireReleasesNativeFrameAndClosesLocalState() {
    val failure = IllegalArgumentException("frame metadata overflow")
    var released = 0
    var closed = 0

    val thrown =
      assertFailsWith<IllegalArgumentException> {
        FrameAcquirePolicy.cleanupAfterWrapperFailure(
          acquired = true,
          releaseNative = { released += 1 },
          closeLocal = { closed += 1 },
          failure = failure,
        )
      }

    assertSame(failure, thrown)
    assertEquals(1, released)
    assertEquals(1, closed)
  }

  @Test
  fun wrapperFailureBeforeNativeAcquireOnlyClosesLocalState() {
    val failure = IllegalArgumentException("native acquire failed")
    var released = 0
    var closed = 0

    val thrown =
      assertFailsWith<IllegalArgumentException> {
        FrameAcquirePolicy.cleanupAfterWrapperFailure(
          acquired = false,
          releaseNative = { released += 1 },
          closeLocal = { closed += 1 },
          failure = failure,
        )
      }

    assertSame(failure, thrown)
    assertEquals(0, released)
    assertEquals(1, closed)
  }

  @Test
  fun wrapperFailureStillClosesLocalStateWhenNativeFrameReleaseFails() {
    val failure = IllegalArgumentException("frame copy failed")
    var closed = 0

    val thrown =
      assertFailsWith<IllegalArgumentException> {
        FrameAcquirePolicy.cleanupAfterWrapperFailure(
          acquired = true,
          releaseNative = { throw IllegalStateException("wrong thread") },
          closeLocal = { closed += 1 },
          failure = failure,
        )
      }

    assertSame(failure, thrown)
    assertEquals(1, closed)
  }

  @Test
  fun localCleanupFailureDoesNotReplaceOriginalFailure() {
    val failure = IllegalArgumentException("frame copy failed")
    var released = 0

    val thrown =
      assertFailsWith<IllegalArgumentException> {
        FrameAcquirePolicy.cleanupAfterWrapperFailure(
          acquired = true,
          releaseNative = { released += 1 },
          closeLocal = { throw IllegalStateException("cleanup failed") },
          failure = failure,
        )
      }

    assertSame(failure, thrown)
    assertEquals(1, released)
  }

  @Test
  fun cleanupNativeFailureDoesNotReplaceOriginalNativeDiagnostic() {
    memScoped {
      val failure =
        assertFailsWith<InvalidArgumentException> { Status.check(mln_network_status_set(999_999U)) }
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
                  replacement.length.toULong(),
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
