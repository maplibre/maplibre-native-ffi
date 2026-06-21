package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertSame

class FrameReleasePolicyTest : org.maplibre.nativeffi.NativeTestBase() {
  @Test
  fun successfulReleaseClosesLocalState() {
    var released = false
    var locallyClosed = false

    FrameReleasePolicy.close(
      isClosed = { locallyClosed },
      releaseNative = { released = true },
      ownerClosed = { false },
      closeLocal = { locallyClosed = true },
    )

    assertEquals(true, released)
    assertEquals(true, locallyClosed)
  }

  @Test
  fun liveOwnerReleaseFailureLeavesLocalStateRetryable() {
    val failure = IllegalStateException("wrong thread")
    var attempts = 0
    var locallyClosed = false

    val thrown =
      assertFailsWith<IllegalStateException> {
        FrameReleasePolicy.close(
          isClosed = { locallyClosed },
          releaseNative = {
            attempts += 1
            throw failure
          },
          ownerClosed = { false },
          closeLocal = { locallyClosed = true },
        )
      }

    assertSame(failure, thrown)
    assertEquals(1, attempts)
    assertEquals(false, locallyClosed)
  }

  @Test
  fun closedOwnerReleaseFailureConsumesLocalState() {
    var locallyClosed = false

    FrameReleasePolicy.close(
      isClosed = { locallyClosed },
      releaseNative = { throw IllegalStateException("owner closed") },
      ownerClosed = { true },
      closeLocal = { locallyClosed = true },
    )

    assertEquals(true, locallyClosed)
  }
}
