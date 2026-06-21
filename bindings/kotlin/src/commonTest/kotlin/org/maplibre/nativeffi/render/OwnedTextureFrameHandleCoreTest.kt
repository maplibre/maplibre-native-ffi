package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertSame
import kotlin.test.assertTrue

class OwnedTextureFrameHandleCoreTest {
  @Test
  fun closeReleasesNativeThenLocalState() {
    val core = OwnedTextureFrameHandleCore("TestFrameHandle", "test frame is closed")
    var nativeReleases = 0
    var localReleases = 0

    core.close(
      releaseNative = { nativeReleases++ },
      ownerClosed = { false },
      releaseLocal = { localReleases++ },
    )
    core.close(
      releaseNative = { nativeReleases++ },
      ownerClosed = { false },
      releaseLocal = { localReleases++ },
    )

    assertTrue(core.isClosed())
    assertEquals(1, nativeReleases)
    assertEquals(1, localReleases)
  }

  @Test
  fun liveOwnerNativeReleaseFailureLeavesLocalStateRetryable() {
    val core = OwnedTextureFrameHandleCore("TestFrameHandle", "test frame is closed")
    val failure = IllegalStateException("wrong thread")
    var nativeReleases = 0
    var localReleases = 0

    val thrown =
      assertFailsWith<IllegalStateException> {
        core.close(
          releaseNative = {
            nativeReleases++
            throw failure
          },
          ownerClosed = { false },
          releaseLocal = { localReleases++ },
        )
      }

    assertSame(failure, thrown)
    assertEquals(1, nativeReleases)
    assertEquals(0, localReleases)
    assertEquals(false, core.isClosed())
  }

  @Test
  fun closedOwnerNativeReleaseFailureConsumesLocalState() {
    val core = OwnedTextureFrameHandleCore("TestFrameHandle", "test frame is closed")
    var localReleases = 0

    core.close(
      releaseNative = { throw IllegalStateException("owner closed") },
      ownerClosed = { true },
      releaseLocal = { localReleases++ },
    )

    assertTrue(core.isClosed())
    assertEquals(1, localReleases)
  }

  @Test
  fun ensureOpenAndLeakReportUseConfiguredMessages() {
    val core = OwnedTextureFrameHandleCore("TestFrameHandle", "test frame is closed")
    val leaks = mutableListOf<String>()

    core.reportLeak { leaks += it }
    val error =
      assertFailsWith<IllegalStateException> {
        core.close(releaseNative = {}, ownerClosed = { false }, releaseLocal = {})
        core.ensureOpen()
      }

    assertEquals("test frame is closed", error.message)
    assertEquals(
      listOf(
        "Leaked TestFrameHandle; close frame handles explicitly on the render session owner thread."
      ),
      leaks,
    )
  }
}
