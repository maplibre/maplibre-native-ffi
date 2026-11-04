package org.maplibre.nativeffi.render

import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.concurrent.atomics.update
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.CustomGeometrySourceOptions
import org.maplibre.nativeffi.GpuSync
import org.maplibre.nativeffi.GpuSyncKind
import org.maplibre.nativeffi.MapMode
import org.maplibre.nativeffi.MaplibreException
import org.maplibre.nativeffi.RenderDriverKind
import org.maplibre.nativeffi.RenderResult
import org.maplibre.nativeffi.RenderSessionAttachOptions
import org.maplibre.nativeffi.RenderSessionState
import org.maplibre.nativeffi.Status
import org.maplibre.nativeffi.assertFailsWithStatus
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.useReleasing

/**
 * Render sessions on the build's backend: an offscreen session where a test only reads pixels back
 * or needs a session, and a caller-driven borrowed ring of the host's textures where it takes
 * frames.
 */
class RenderSessionTest {
  @Test
  fun anOffscreenFrameReadsBackAsPixels() = runSuspendTest {
    withOffscreenSession(width = 32, height = 16) {
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

  @OptIn(ExperimentalAtomicApi::class)
  @Test
  fun aCustomSourceAnswersThroughTheMapItBorrows() = runSuspendTest {
    // A continuous map asks for its tiles as it renders.
    withOffscreenSession(mapMode = MapMode.CONTINUOUS) {
      val map = mapFixture.map
      val calls = AtomicReference(emptyList<Triple<ULong, String, Status?>>())
      setStyle(RED_BACKGROUND_STYLE)
      complete(
        map.addCustomGeometrySource(
          "custom",
          CustomGeometrySourceOptions(
            fetchTile = { borrowed, sourceId, tile ->
              borrowed.discarding.setCustomGeometrySourceTileData(
                sourceId,
                tile,
                """{"type":"FeatureCollection","features":[]}""",
              )
              // The callback borrows the map, so it can't release it.
              val refusal =
                try {
                  borrowed.dispose()
                  null
                } catch (error: MaplibreException) {
                  error.status
                }
              calls.update { it + Triple(borrowed.id, sourceId, refusal) }
            },
          ),
        ),
        "the source",
      )
      complete(
        map.addStyleLayerJson("""{"id":"dots","type":"circle","source":"custom"}"""),
        "the layer",
      )
      renderUntil("a tile fetch") { calls.load().isNotEmpty() }
      assertEquals(Triple(map.id, "custom", Status.INVALID_STATE), calls.load().first())
      assertFalse(map.isClosed)
    }
  }

  @Test
  fun anAcquiredFrameNamesTheTextureTheSessionRendered() = runSuspendTest {
    withBorrowedTexture(width = 32, height = 16) {
      setStyle(RED_BACKGROUND_STYLE)
      renderStill()
      // One more frame, left for the test to acquire.
      assertEquals(RenderResult.RENDERED, renderFrame().disposition)

      val frame = assertNotNull(session.acquireFrame())
      assertEquals(32u to 16u, TestGraphics.frameTextureSize(frame))
      frame.release(GpuSync(kind = GpuSyncKind.CPU_COMPLETE))
    }
  }

  @Test
  fun aDrainBeforeAnyDemandReturnsNoResults() = runSuspendTest {
    withBorrowedTexture {
      // Native writes a null batch and a null frame, which read as no results and no frame.
      assertEquals(emptyList(), session.drainFrameResults())
      assertNull(session.acquireFrame())
    }
  }

  @Test
  fun aBorrowedFrameViewExpiresWithItsBlockAndHoldsOffItsRelease() = runSuspendTest {
    // Two textures, since the test holds two frames at once.
    withBorrowedTexture(ringDepth = 2) {
      setStyle(RED_BACKGROUND_STYLE)
      renderStill()
      assertEquals(RenderResult.RENDERED, renderFrame().disposition)
      val frame = assertNotNull(session.acquireFrame())
      assertEquals(RenderResult.RENDERED, renderFrame().disposition)
      val sibling = assertNotNull(session.acquireFrame())

      var escaped: GpuSync? = null
      var kind: GpuSyncKind? = null
      frame.withProducerSync { sync ->
        escaped = sync
        kind = sync.kind
        val handle = sync.handle
        // The view borrows the frame, so neither the frame nor its session can go away under it.
        assertFailsWithStatus(Status.BUSY) {
          frame.release(GpuSync(kind = GpuSyncKind.CPU_COMPLETE))
        }
        assertFailsWithStatus(Status.BUSY) { session.abandon() }
        // Discarding another frame leaves this view readable until its block ends.
        assertFalse(sibling.isClosed)
        sibling.discard()
        assertTrue(sibling.isClosed)
        sibling.discard()
        assertEquals(handle, sync.handle)
      }
      // Only the sync object that the frame lends expires with the block.
      assertEquals(kind, escaped!!.kind)
      assertFailsWith<IllegalStateException> { escaped.handle }
      frame.release(GpuSync(kind = GpuSyncKind.CPU_COMPLETE))
      assertFailsWithStatus(Status.INVALID_STATE) { frame.withProducerSync {} }
    }
  }

  @Test
  fun useDisposesAnAttachedSessionAndItsMapReleasesAfterIt() = runSuspendTest {
    withOffscreenSession {
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
  fun useReleasingAwaitsTheReleaseOfAnAttachedSession() = runSuspendTest {
    withOffscreenSession {
      session.useReleasing {
        setStyle(RED_BACKGROUND_STYLE)
        renderStill()
      }
      // Native refuses an attach while the map still has a session.
      val next =
        attachOffscreenSession(
          mapFixture.map,
          32,
          16,
          RenderSessionAttachOptions(driver = RenderDriverKind.CORE_WORKER),
        )
      next.use { next.ready.awaitWithin("the next attach") }
    }
  }
}

/** A style that paints every pixel opaque red. */
internal const val RED_BACKGROUND_STYLE =
  """{"version":8,"sources":{},"layers":[{"id":"bg","type":"background","paint":{"background-color":"#ff0000"}}]}"""
