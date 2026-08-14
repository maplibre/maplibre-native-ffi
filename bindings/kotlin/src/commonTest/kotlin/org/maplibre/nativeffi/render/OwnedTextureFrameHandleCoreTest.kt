package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertSame
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus

class OwnedTextureFrameHandleCoreTest {
  @Test
  fun closeReleasesNativeThenLocalState(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val core = OwnedTextureFrameHandleCore("TestFrameHandle")
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
  fun liveOwnerNativeReleaseFailureLeavesLocalStateRetryable(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val core = OwnedTextureFrameHandleCore("TestFrameHandle")
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
  fun closedOwnerNativeReleaseFailureConsumesLocalState(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val core = OwnedTextureFrameHandleCore("TestFrameHandle")
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
  fun closedFrameRejectsUseWithTheSharedReleasedHandleError(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val core = OwnedTextureFrameHandleCore("TestFrameHandle")
      val leaks = mutableListOf<String>()

      core.reportLeak { leaks += it }
      core.close(releaseNative = {}, ownerClosed = { false }, releaseLocal = {})
      val error = assertFailsWith<InvalidStateException> { core.ensureOpen() }

      assertEquals(MaplibreStatus.INVALID_STATE, error.status)
      assertEquals("TestFrameHandle is already closed", error.diagnostic)
      assertEquals(
        listOf(
          "Leaked TestFrameHandle; close frame handles explicitly on the render session graphics thread."
        ),
        leaks,
      )
    }
}
