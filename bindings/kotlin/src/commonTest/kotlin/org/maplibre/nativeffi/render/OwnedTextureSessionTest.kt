package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.assertFailsWithStatus
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.generated.GpuSync
import org.maplibre.nativeffi.generated.RenderResult
import org.maplibre.nativeffi.generated.RenderSessionState
import org.maplibre.nativeffi.generated.Status
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.runtime.use as useAndAwaitRelease

/**
 * The render session shapes over an owned texture on the build's backend, which the caller drives
 * unless [TestGraphics.readbackDriver] names a core worker.
 */
class OwnedTextureSessionTest {
  @Test
  fun anOwnedTextureFrameReadsBackAsPixels() = runSuspendTest {
    withOwnedTexture(width = 32, height = 16, driver = TestGraphics.readbackDriver) {
      setStyle(RED_BACKGROUND_STYLE)
      renderStill()
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

      complete(session.detach())
      assertEquals(RenderSessionState.DETACHED, session.getSnapshot().state)
    }
  }

  @Test
  fun anAcquiredFrameNamesTheTextureTheSessionRendered() = runSuspendTest {
    withOwnedTexture(width = 32, height = 16) {
      setStyle(RED_BACKGROUND_STYLE)
      renderStill()
      // One more frame, left for the test to acquire.
      assertEquals(RenderResult.RENDERED, renderFrame().disposition)

      val frame = assertNotNull(session.acquireFrame())
      assertEquals(32u to 16u, TestGraphics.frameTextureSize(frame))
      frame.release()
    }
  }

  @Test
  fun aDrainBeforeAnyDemandReturnsNull() = runSuspendTest {
    withOwnedTexture {
      // Native reports both calls as not ready, which reads as null.
      assertNull(session.drainFrameResults())
      assertNull(session.acquireFrame())
    }
  }

  @Test
  fun aBorrowedFrameViewExpiresWithItsBlockAndHoldsOffItsRelease() = runSuspendTest {
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
        // Discarding another frame leaves this view readable until its block ends.
        sibling.discard()
        assertEquals(kind, sync.kind)
      }
      assertFailsWith<IllegalStateException> { escaped!!.kind }
      frame.release()
      assertFailsWithStatus(Status.INVALID_STATE) { frame.withProducerSync {} }
    }
  }

  @Test
  fun useDisposesAnAttachedSessionAndItsMapReleasesAfterIt() = runSuspendTest {
    withOwnedTexture {
      session.use {
        setStyle(RED_BACKGROUND_STYLE)
        renderStill()
      }
      assertTrue(session.isClosed)
      // The dispose reports nothing, and the map release completes once the session has retired.
      mapFixture.map.release().awaitWithin("the map release")
    }
  }

  @Test
  fun suspendingUseAwaitsTheReleaseOfAnAttachedSession() = runSuspendTest {
    withOwnedTexture {
      // The suspending `use`, imported under another name so `use` stays AutoCloseable's.
      session.useAndAwaitRelease {
        setStyle(RED_BACKGROUND_STYLE)
        renderStill()
      }
      assertTrue(session.isClosed)
      mapFixture.map.release().awaitWithin("the map release")
    }
  }

  @Test
  fun aCallerDrivenSessionIsServicedFromTheGraphicsThread() = runSuspendTest {
    withOwnedTexture {
      setStyle(RED_BACKGROUND_STYLE)
      // The fixture services the session from a coroutine on TestGraphics.thread, woken by the
      // session's driver-work and frame wakes: a thread of its own on the threaded targets, and the
      // page's event loop on wasm.
      renderStill()
      assertEquals(RenderResult.RENDERED, renderFrame().disposition)
    }
  }
}

/** A style that paints every pixel opaque red. */
internal const val RED_BACKGROUND_STYLE =
  """{"version":8,"sources":{},"layers":[{"id":"bg","type":"background","paint":{"background-color":"#ff0000"}}]}"""
