package org.maplibre.nativeffi.internal.lifecycle

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith

class BorrowedResourceCoreTest {
  @Test
  fun closeWithoutBorrowsReleasesNativeOnce(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      var releases = 0
      val core = BorrowedResourceCore("TestResource") { releases++ }

      core.close()
      core.close()
      core.releaseNativeForCleaner()

      assertEquals(1, releases)
    }

  @Test
  fun closeDuringBorrowDefersNativeReleaseUntilBorrowEnds(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      var releases = 0
      val core = BorrowedResourceCore("TestResource") { releases++ }

      core.withOpenResource {
        core.close()
        assertEquals(0, releases)
      }

      assertEquals(1, releases)
    }

  @Test
  fun borrowAfterCloseFails(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val core = BorrowedResourceCore("TestResource") {}

      core.close()

      assertFailsWith<IllegalStateException> { core.withOpenResource {} }
    }

  @Test
  fun cleanerReleaseIsIdempotentWithClose(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      var releases = 0
      val core = BorrowedResourceCore("TestResource") { releases++ }

      core.releaseNativeForCleaner()
      core.close()

      assertEquals(1, releases)
    }
}
