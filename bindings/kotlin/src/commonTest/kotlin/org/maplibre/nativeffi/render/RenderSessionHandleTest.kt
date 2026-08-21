package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertSame
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.UnsupportedFeatureException
import org.maplibre.nativeffi.error.WrongThreadException
import org.maplibre.nativeffi.failureFromBackgroundThread
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenBox
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.sleepMillis

class RenderSessionHandleTest {
  // BND-160, BND-161, BND-163, BND-164, BND-165, BND-166, BND-167, BND-168,
  // BND-169, BND-170: owned-texture rendering, readback, frames, and
  // owner-thread checks.

  @Test
  fun renderUpdateWithoutPendingUpdateReportsNoUpdateAndKeepsSessionLive() {
    withOwnedTextureSession(mapMode = MapMode.STATIC) { _, _, owned ->
      val session = owned.session
      assertEquals(RenderResult.NO_UPDATE, session.renderUpdate().result)
      session.resize(32, 16, 1.0)
    }
  }

  @Test
  fun renderUpdateReportsNeedsRepaintDuringCameraTransition() {
    Maplibre.setLogCallback(LogCallback { true })
    Maplibre.setAsyncLogSeverities(emptySet())
    try {
      withOwnedTextureSession { runtime, map, owned ->
        val session = owned.session
        map.setStyleJson(BACKGROUND_STYLE_JSON.encodeToByteArray())

        // The session owns this thread, which is also the map's, so the map
        // only reaches the style when this loop pumps the runtime. Render until
        // the map settles: the last frame asks for no repaint.
        var update = session.renderUpdate()
        for (attempt in 0 until 500) {
          if (update.result == RenderResult.RENDERED && !update.needsRepaint) break
          runtime.pump(0)
          update = session.renderUpdate()
          if (update.result != RenderResult.RENDERED || update.needsRepaint) {
            sleepMillis(1)
          }
        }
        assertEquals(RenderResult.RENDERED, update.result)
        assertFalse(update.needsRepaint)

        map.easeTo(
          CameraOptions().apply { zoom = 4.0 },
          AnimationOptions().apply { durationMs = 60_000.0 },
        )

        var sawRepaintRequest = false
        for (attempt in 0 until 500) {
          runtime.pump(0)
          update = session.renderUpdate()
          if (update.result == RenderResult.RENDERED && update.needsRepaint) {
            sawRepaintRequest = true
            break
          }
          sleepMillis(1)
        }
        assertTrue(sawRepaintRequest)
      }
    } finally {
      Maplibre.clearLogCallback()
      Maplibre.restoreDefaultAsyncLogSeverities()
    }
  }

  @Test
  fun ownedTextureSessionRendersReadsBackAcquiresFrameAndDetaches() {
    Maplibre.setLogCallback(LogCallback { true })
    Maplibre.setAsyncLogSeverities(emptySet())
    try {
      withOwnedTextureSession { runtime, map, owned ->
        val session = owned.session
        val featureCoordinate = LatLng(37.7749, -122.4194)
        map.jumpTo(CameraOptions().apply { center = featureCoordinate })
        assertSame(map, session.map())
        assertFailsWith<InvalidStateException> { session.textureImageInfo() }
        session.setFeatureState(featureStateSelector(), featureState())
        val queuedState = session.getFeatureState(featureStateSelector())
        assertEquals("true", rawMember(queuedState, "hover")?.decodeToString())
        assertEquals(20.0, numberMember(queuedState, "radius"))
        assertFailsWith<InvalidStateException> { map.close() }
        assertFailsWith<InvalidStateException> { owned.attachAnotherOwnedTexture(16, 8).close() }

        assertTrue(waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE))
        session.renderUpdate()
        val beforeStyle = session.getFeatureState(featureStateSelector())
        assertEquals("true", rawMember(beforeStyle, "hover")?.decodeToString())
        assertEquals(20.0, numberMember(beforeStyle, "radius"))

        map.setStyleJson(QUERY_STYLE_JSON.encodeToByteArray())
        assertTrue(waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE))
        assertEquals(RenderResult.RENDERED, session.renderUpdate().result)

        val sessionCallWrongThread = failureFromBackgroundThread { session.renderUpdate() }
        if (sessionCallWrongThread !is WrongThreadException) throw sessionCallWrongThread
        val sessionCallDiagnostic = sessionCallWrongThread.diagnostic
        assertEquals(MaplibreStatus.WRONG_THREAD, sessionCallWrongThread.status)
        assertTrue(sessionCallDiagnostic.isNotBlank())

        assertEquals(RenderResult.RENDERED, session.renderUpdate().result)
        assertEquals(sessionCallDiagnostic, sessionCallWrongThread.diagnostic)

        val sessionCloseWrongThread = failureFromBackgroundThread { session.close() }
        if (sessionCloseWrongThread !is WrongThreadException) throw sessionCloseWrongThread
        assertEquals(MaplibreStatus.WRONG_THREAD, sessionCloseWrongThread.status)
        assertFalse(session.isClosed)

        val info = session.textureImageInfo()
        assertEquals(32, info.width)
        assertEquals(16, info.height)
        assertEquals(32 * 4, info.stride)
        assertEquals(info.stride.toLong() * info.height.toLong(), info.byteLength)

        NativeBuffer.allocate(4).use { small ->
          assertFailsWith<InvalidArgumentException> { session.readPremultipliedRgba8(small) }
        }
        NativeBuffer.allocate(info.byteLength).use { buffer ->
          assertEquals(info, session.readPremultipliedRgba8(buffer))
          assertEquals(info.byteLength.toInt(), buffer.toByteArray().size)
        }

        assertFailsWith<InvalidArgumentException> {
          session.setFeatureState(featureStateSelector(), jsonBytes("[]"))
        }
        session.setFeatureState(featureStateSelector(), featureState())
        val copiedState = session.getFeatureState(featureStateSelector())
        assertEquals("true", rawMember(copiedState, "hover")?.decodeToString())
        assertEquals(20.0, numberMember(copiedState, "radius"))

        renderIfAvailable(runtime, map, session)
        session.removeFeatureState(
          FeatureStateSelector("point").apply {
            featureId = "feature-1"
            stateKey = "hover"
          }
        )
        renderIfAvailable(runtime, map, session)
        val afterRemove = session.getFeatureState(featureStateSelector())
        assertEquals(null, rawMember(afterRemove, "hover"))
        assertEquals(20.0, numberMember(afterRemove, "radius"))

        val queryGeometry =
          RenderedQueryGeometry.Box(ScreenBox(ScreenPoint(0.0, 0.0), ScreenPoint(1.0, 1.0)))
        val frame = owned.acquireFrame()
        try {
          assertEquals(32, frame.width)
          assertEquals(16, frame.height)
          assertFalse(frame.isClosed)
          assertFailsWith<InvalidStateException> { session.renderUpdate() }
          assertFailsWith<InvalidStateException> { session.resize(16, 8, 2.0) }
          assertFailsWith<InvalidStateException> {
            session.setMetalBorrowedTextureTarget(dummyMetalBorrowedTexture())
          }
          assertFailsWith<InvalidStateException> {
            session.setVulkanBorrowedTextureTarget(dummyVulkanBorrowedTexture())
          }
          assertFailsWith<InvalidStateException> {
            session.setOpenGLBorrowedTextureTarget(dummyOpenGLBorrowedTexture())
          }
          assertFailsWith<InvalidStateException> { session.detach() }
          assertFailsWith<InvalidStateException> { session.reduceMemoryUse() }
          assertFailsWith<InvalidStateException> { session.clearData() }
          assertFailsWith<InvalidStateException> { session.dumpDebugLogs() }
          assertFailsWith<InvalidStateException> {
            session.setFeatureState(featureStateSelector(), jsonBytes("true"))
          }
          assertFailsWith<InvalidStateException> { session.getFeatureState(featureStateSelector()) }
          assertFailsWith<InvalidStateException> {
            session.removeFeatureState(featureStateSelector())
          }
          assertFailsWith<InvalidStateException> {
            session.queryRenderedFeatures(queryGeometry, null)
          }
          assertFailsWith<InvalidStateException> { session.querySourceFeatures("point", null) }
          assertFailsWith<InvalidStateException> {
            session.queryFeatureExtension(
              "point",
              jsonBytes("""{"type":"Feature","geometry":{"type":"Point","coordinates":[0,0]}}"""),
              "supercluster",
              "children",
              null,
            )
          }
          assertFailsWith<InvalidStateException> { session.textureImageInfo() }
          NativeBuffer.allocate(1).use { buffer ->
            assertFailsWith<InvalidStateException> { session.readPremultipliedRgba8(buffer) }
          }
          assertFailsWith<InvalidStateException> { owned.acquireFrame() }
          assertFailsWith<InvalidStateException> { session.close() }

          val closeError = failureFromBackgroundThread { frame.close() }
          assertTrue(closeError is WrongThreadException)
          assertFalse(frame.isClosed)
          assertFailsWith<InvalidStateException> { session.renderUpdate() }
        } finally {
          frame.close()
        }
        assertTrue(frame.isClosed)
        assertFailsWith<IllegalStateException> { frame.width }

        // Resizing hands the new logical size to the map's owner thread, so
        // the map publishes an update matching the new target only once
        // pumped.
        session.resize(16, 8, 2.0)
        assertEquals(RenderResult.SIZE_PENDING, session.renderUpdate().result)
        runtime.pump(0)
        assertEquals(RenderResult.RENDERED, session.renderUpdate().result)
        session.detach()
        assertFailsWith<InvalidStateException> { session.renderUpdate() }
        assertFalse(session.isClosed)
      }
    } finally {
      Maplibre.clearLogCallback()
      Maplibre.restoreDefaultAsyncLogSeverities()
    }
  }

  // BND-176: set_target reports unsupported for a target kind the session does
  // not have. Dummy descriptors are enough: native checks the session kind
  // before it reads GPU objects.

  @Test
  fun ownedTextureSetTargetReportsUnsupportedForOtherTargetKinds() {
    withOwnedTextureSession(mapMode = MapMode.STATIC) { _, _, owned ->
      val session = owned.session
      val error =
        assertFailsWith<UnsupportedFeatureException> {
          session.setMetalBorrowedTextureTarget(dummyMetalBorrowedTexture())
        }
      assertEquals(MaplibreStatus.UNSUPPORTED, error.status)
      assertFailsWith<UnsupportedFeatureException> {
        session.setVulkanBorrowedTextureTarget(dummyVulkanBorrowedTexture())
      }
      assertFailsWith<UnsupportedFeatureException> {
        session.setOpenGLBorrowedTextureTarget(dummyOpenGLBorrowedTexture())
      }
      assertFailsWith<UnsupportedFeatureException> {
        session.setMetalSurfaceTarget(dummyMetalSurface())
      }
      assertFailsWith<UnsupportedFeatureException> {
        session.setVulkanSurfaceTarget(dummyVulkanSurface())
      }
      assertFailsWith<UnsupportedFeatureException> {
        session.setOpenGLSurfaceTarget(dummyOpenGLSurface())
      }
    }
  }

  private fun featureStateSelector(): FeatureStateSelector =
    FeatureStateSelector("point").apply { featureId = "feature-1" }

  private fun featureState(): ByteArray = jsonBytes("""{"hover":true,"radius":20}""")

  private companion object {
    private const val BACKGROUND_STYLE_JSON =
      """{"version":8,"sources":{},"layers":[{"id":"bg","type":"background","paint":{"background-color":"#ff0000"}}]}"""
  }
}

private fun dummyPointer(): NativePointer = NativePointer.ofAddress(1)

private fun dummyMetalBorrowedTexture(): MetalBorrowedTextureDescriptor =
  MetalBorrowedTextureDescriptor(RenderTargetExtent(16, 8, 1.0), 16, 8, dummyPointer())

private fun dummyMetalSurface(): MetalSurfaceDescriptor =
  MetalSurfaceDescriptor(
    RenderTargetExtent(16, 8, 1.0),
    MetalContextDescriptor(dummyPointer()),
    dummyPointer(),
  )

private fun dummyVulkanContext(): VulkanContextDescriptor =
  VulkanContextDescriptor(
    dummyPointer(),
    dummyPointer(),
    dummyPointer(),
    dummyPointer(),
    0,
    dummyPointer(),
    dummyPointer(),
  )

private fun dummyVulkanBorrowedTexture(): VulkanBorrowedTextureDescriptor =
  VulkanBorrowedTextureDescriptor(
    RenderTargetExtent(16, 8, 1.0),
    16,
    8,
    dummyVulkanContext(),
    dummyPointer(),
    dummyPointer(),
    0,
    0,
  )

private fun dummyVulkanSurface(): VulkanSurfaceDescriptor =
  VulkanSurfaceDescriptor(RenderTargetExtent(16, 8, 1.0), dummyVulkanContext(), dummyPointer())

private fun dummyEglContext(): EglContextDescriptor =
  EglContextDescriptor(dummyPointer(), dummyPointer(), dummyPointer(), NativePointer.NULL_POINTER)

private fun dummyOpenGLBorrowedTexture(): OpenGLBorrowedTextureDescriptor =
  OpenGLBorrowedTextureDescriptor(
    RenderTargetExtent(16, 8, 1.0),
    16,
    8,
    dummyEglContext(),
    1,
    0x0DE1,
  )

private fun dummyOpenGLSurface(): OpenGLSurfaceDescriptor =
  OpenGLSurfaceDescriptor(RenderTargetExtent(16, 8, 1.0), dummyEglContext(), dummyPointer())
