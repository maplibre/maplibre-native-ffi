package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNotNull
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.runBlocking
import org.maplibre.nativeffi.RenderResult
import org.maplibre.nativeffi.Status
import org.maplibre.nativeffi.TestThread
import org.maplibre.nativeffi.assertFailsWithStatus
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.runSuspendTest

/** A frame view borrowed on a second Kotlin thread, over the build's owned texture. */
class FrameBorrowThreadTest {
  @Test
  fun aBorrowOnAnotherThreadHoldsOffTheFramesReleaseUntilItEnds() = runSuspendTest {
    withOwnedTexture {
      setStyle(RED_BACKGROUND_STYLE)
      renderStill()
      assertEquals(RenderResult.RENDERED, renderFrame().disposition)
      val frame = assertNotNull(session.acquireFrame())
      val entered = CompletableDeferred<Unit>()
      val leave = CompletableDeferred<Unit>()
      val borrower = TestThread {
        frame.withProducerSync {
          entered.complete(Unit)
          runBlocking { leave.await() }
        }
      }
      try {
        entered.awaitWithin("the borrow to start on the other thread")
        assertFailsWithStatus(Status.BUSY) { frame.release(null) }
      } finally {
        leave.complete(Unit)
        borrower.join()
      }
      frame.release(null)
      assertFailsWithStatus(Status.INVALID_STATE) { frame.withProducerSync {} }
    }
  }
}
