package org.maplibre.nativeffi.internal.lifecycle

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith

class BorrowedResourceCoreTest {
  @Test
  fun closeWithoutBorrowsReleasesNativeOnce() {
    var releases = 0
    val core = BorrowedResourceCore("TestResource") { releases++ }

    core.close()
    core.close()
    core.releaseNativeForCleaner()

    assertEquals(1, releases)
  }

  @Test
  fun closeDuringBorrowDefersNativeReleaseUntilBorrowEnds() {
    var releases = 0
    val core = BorrowedResourceCore("TestResource") { releases++ }

    core.withOpenResource {
      core.close()
      assertEquals(0, releases)
    }

    assertEquals(1, releases)
  }

  @Test
  fun borrowAfterCloseFails() {
    val core = BorrowedResourceCore("TestResource") {}

    core.close()

    assertFailsWith<IllegalStateException> { core.withOpenResource {} }
  }

  @Test
  fun cleanerReleaseIsIdempotentWithClose() {
    var releases = 0
    val core = BorrowedResourceCore("TestResource") { releases++ }

    core.releaseNativeForCleaner()
    core.close()

    assertEquals(1, releases)
  }
}
