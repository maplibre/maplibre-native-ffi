package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertSame

class FrameAcquirePolicyTest {
  @Test
  fun wrapperFailureAfterNativeAcquireReleasesNativeFrameAndClosesLocalState(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
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
  fun wrapperFailureBeforeNativeAcquireOnlyClosesLocalState(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
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
  fun wrapperFailureStillClosesLocalStateWhenNativeFrameReleaseFails(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
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
  fun localCleanupFailureDoesNotReplaceOriginalFailure(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
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
}
