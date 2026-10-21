package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.runBlocking
import org.maplibre.nativeffi.TestThread
import org.maplibre.nativeffi.assertFailsWithStatus
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.generated.GpuSync
import org.maplibre.nativeffi.generated.RenderResult
import org.maplibre.nativeffi.generated.RenderSessionState
import org.maplibre.nativeffi.generated.Status
import org.maplibre.nativeffi.runSuspendTest

/** The render session shapes over a caller-driven owned texture on the build's backend. */
class OwnedTextureSessionTest {
  @Test
  fun anOwnedTextureFrameReadsBackAsPixels(): Unit = runSuspendTest {
    withOwnedTexture(width = 32, height = 16) {
      setStyle(RED_BACKGROUND_STYLE)
      renderStill()
      // One more frame, left for the test to acquire below.
      assertEquals(RenderResult.RENDERED, renderFrame().disposition)

      val readback = complete(session.readTexture())
      assertEquals(32u, readback.info.width)
      assertEquals(16u, readback.info.height)
      assertEquals(readback.info.stride.toULong() * 16uL, readback.info.byteLength)
      assertEquals(readback.info.byteLength.toInt(), readback.data.size)
      val center = 8 * readback.info.stride.toInt() + 16 * 4
      assertEquals(
        listOf(255, 0, 0, 255),
        readback.data.copyOfRange(center, center + 4).map { it.toInt() and 0xff },
      )

      // The typed texture view of the acquired frame names the texture the session rendered.
      val frame = assertNotNull(session.acquireFrame())
      assertEquals(32u to 16u, TestGraphics.frameTextureSize(frame))
      frame.release()

      complete(session.detach())
      assertEquals(RenderSessionState.DETACHED, session.getSnapshot().state)
    }
  }

  @Test
  fun aDrainBeforeAnyDemandReturnsNull(): Unit = runSuspendTest {
    withOwnedTexture {
      // Native reports both calls as not ready, which reads as null.
      assertNull(session.drainFrameResults())
      assertNull(session.acquireFrame())
    }
  }

  @Test
  fun aBorrowedFrameViewExpiresWithItsBlockAndHoldsOffItsRelease(): Unit = runSuspendTest {
    withOwnedTexture(textureRingDepth = 2u) {
      setStyle(RED_BACKGROUND_STYLE)
      renderStill()
      assertEquals(RenderResult.RENDERED, renderFrame().disposition)
      val frame = assertNotNull(session.acquireFrame())
      assertEquals(RenderResult.RENDERED, renderFrame().disposition)
      val sibling = assertNotNull(session.acquireFrame())

      var escaped: GpuSync? = null
      frame.withProducerSync { sync ->
        escaped = sync
        val kind = sync.kind
        // The view borrows the frame, so neither the frame nor its session can go away under it.
        assertFailsWithStatus(Status.BUSY) { frame.release() }
        assertFailsWithStatus(Status.BUSY) { session.abandon() }
        // Disposing another frame leaves this view readable until its block ends.
        sibling.dispose()
        assertEquals(kind, sync.kind)
      }
      assertFailsWith<IllegalStateException> { escaped!!.kind }
      frame.release()
      assertFailsWithStatus(Status.INVALID_STATE) { frame.withProducerSync {} }
    }
  }

  @Test
  fun aBorrowOnAnotherThreadHoldsOffTheFramesReleaseUntilItEnds(): Unit = runSuspendTest {
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
        assertFailsWithStatus(Status.BUSY) { frame.release() }
      } finally {
        leave.complete(Unit)
        borrower.join()
      }
      frame.release()
      assertFailsWithStatus(Status.INVALID_STATE) { frame.withProducerSync {} }
    }
  }

  @Test
  fun aCallerDrivenSessionStaysOnTheGraphicsThreadItWasServicedFrom(): Unit = runSuspendTest {
    withOwnedTexture {
      setStyle(RED_BACKGROUND_STYLE)
      // The fixture services the session from a coroutine on its own graphics thread, woken by the
      // session's driver-work and frame wakes, while this test runs on another thread.
      renderStill()

      assertFailsWithStatus(Status.WRONG_THREAD) { session.serviceDriverWork() }
    }
  }

  private companion object {
    const val RED_BACKGROUND_STYLE =
      """{"version":8,"sources":{},"layers":[{"id":"bg","type":"background","paint":{"background-color":"#ff0000"}}]}"""
  }
}
