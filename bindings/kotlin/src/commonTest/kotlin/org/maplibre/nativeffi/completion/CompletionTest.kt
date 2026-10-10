package org.maplibre.nativeffi.completion

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertNull
import kotlin.test.assertTrue
import kotlin.time.Duration.Companion.milliseconds
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.async
import kotlinx.coroutines.cancelAndJoin
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.withTimeoutOrNull
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.generated.MapMode
import org.maplibre.nativeffi.generated.OfflineRegionDownloadState
import org.maplibre.nativeffi.pendingCompletionsForTesting
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.runtime.awaitFailed
import org.maplibre.nativeffi.withMap

/** How a native completion reaches Kotlin as a Deferred. */
class CompletionTest {
  @Test
  fun aSubmissionNativeRejectsLeavesNoCompletionBehind(): Unit = runSuspendTest {
    withMap {
      val before = pendingCompletionsForTesting()
      // Native rejects the unknown state before it takes the completion, so the bridge frees its
      // state and fails the Deferred with the rejection.
      val rejected = runtime.setOfflineRegionDownloadState(1, OfflineRegionDownloadState(900u))
      assertEquals(before, pendingCompletionsForTesting())
      val failure = assertFailsWith<InvalidArgumentException> { rejected.await() }
      assertTrue(failure.diagnostic.isNotEmpty(), "the rejection carries its diagnostic")
    }
  }

  @Test
  fun aFailedCommandArrivesAsDataRatherThanAnException(): Unit = runSuspendTest {
    withMap {
      loadStyle()
      // awaitFailed returns the FAILED disposition with its status and diagnostic.
      map.removeStyleSource("missing").awaitFailed(MaplibreStatus.NOT_FOUND)
    }
  }

  @Test
  fun waitingOnACompletionTimesOutOrIsCancelledWithoutEndingIt(): Unit = runSuspendTest {
    withMap(mapMode = MapMode.STATIC) {
      loadStyle()
      // A static map renders its still image only for a render session, and none is attached.
      val still = map.requestStillImage()

      assertNull(withTimeoutOrNull(1.milliseconds) { still.await() })
      coroutineScope {
        val waiter = async(start = CoroutineStart.UNDISPATCHED) { still.await() }
        waiter.cancelAndJoin()
        assertTrue(waiter.isCancelled)
      }
      // Neither the timeout nor the cancelled wait touched the operation, which the map's release
      // ends instead.
      assertTrue(still.isActive)

      map.release().await()
      val failure = assertFailsWith<MaplibreException> { still.await() }
      assertEquals(MaplibreStatus.CANCELLED, failure.status)
    }
  }
}
