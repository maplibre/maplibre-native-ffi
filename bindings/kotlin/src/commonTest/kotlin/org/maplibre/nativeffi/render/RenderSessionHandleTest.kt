package org.maplibre.nativeffi.render

import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertSame
import kotlin.test.assertTrue
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.generated.CameraOptions
import org.maplibre.nativeffi.generated.CameraUpdate
import org.maplibre.nativeffi.generated.EglContextDescriptor
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.MapMode
import org.maplibre.nativeffi.generated.MetalContextDescriptor
import org.maplibre.nativeffi.generated.RenderDriverKind
import org.maplibre.nativeffi.generated.RenderResult
import org.maplibre.nativeffi.generated.RenderSessionCapabilityFlag
import org.maplibre.nativeffi.generated.RenderSessionState
import org.maplibre.nativeffi.generated.RenderTargetExtent
import org.maplibre.nativeffi.generated.ResourceProvider
import org.maplibre.nativeffi.generated.VulkanContextDescriptor
import org.maplibre.nativeffi.runtime.runSuspendTest
import org.maplibre.nativeffi.sleepMillis

class RenderSessionHandleTest {
  @Test
  fun activeGpuViewSurvivesSiblingDisposalAndRejectsSubsequentAccess(): Unit = runSuspendTest {
    withOwnedTextureSession(textureRingDepth = 2u) { runtime, map, owned ->
      val session = owned.session
      session.completeOnDriver(map.setStyleJson(BACKGROUND_STYLE_JSON.encodeToByteArray()))
      session.completeOnDriver(runtime.barrier())
      session.renderUntilSettled()
      val frame = session.acquireFrame()
      assertEquals(RenderResult.RENDERED, session.renderOneFrame().disposition)
      val sibling = session.acquireFrame()
      var retained: GpuSync? = null
      frame.withGetProducerSync { sync ->
        retained = sync
        val kind = sync.kind
        assertEquals(
          MaplibreStatus.BUSY,
          assertFailsWith<MaplibreException> { frame.release() }.status,
        )
        assertEquals(
          MaplibreStatus.BUSY,
          assertFailsWith<MaplibreException> { session.abandon() }.status,
        )
        sibling.dispose()
        assertEquals(kind, sync.kind)
      }
      assertFailsWith<IllegalStateException> { retained!!.kind }
      assertFailsWith<MaplibreException> { frame.withGetProducerSync { it.kind } }
      frame.release()
      session.close()
    }
  }

  @Test
  fun globalStateChangesRenderedPaint(): Unit = runSuspendTest {
    withOwnedTextureSession { runtime, map, owned ->
      val session = owned.session
      session.completeOnDriver(
        map.setStyleJson(
          """{"version":8,"transition":{"duration":0},"state":{"color":{"default":"#ff0000"}},"sources":{},"layers":[{"id":"bg","type":"background","paint":{"background-color":["global-state","color"]}}]}"""
            .encodeToByteArray()
        )
      )
      suspend fun renderColor(): List<Int> {
        session.completeOnDriver(runtime.barrier())
        session.renderUntilSettled()
        return session.completeOnDriver(session.textureReadPremultipliedRgba8()).data.take(4).map {
          it.toInt() and 255
        }
      }
      assertEquals(listOf(255, 0, 0, 255), renderColor())
      session.completeOnDriver(
        map.setGlobalStateProperty("color", "\"#0000ff\"".encodeToByteArray())
      )
      assertEquals(listOf(0, 0, 255, 255), renderColor())
      session.completeOnDriver(map.setGlobalStateProperty("color", "null".encodeToByteArray()))
      assertEquals(listOf(255, 0, 0, 255), renderColor())
      session.completeOnDriver(session.detach())
    }
  }

  @OptIn(ExperimentalAtomicApi::class)
  @Test
  fun staticRenderWaitingForStyleKeepsThePreviousProjection(): Unit = runSuspendTest {
    withOwnedTextureSession(mapMode = MapMode.STATIC) { runtime, map, owned ->
      val session = owned.session
      val pending = AtomicReference<org.maplibre.nativeffi.resource.ResourceRequestHandle?>(null)
      session.completeOnDriver(
        runtime.setResourceProvider(
          ResourceProvider(
            callback = provider@{ _, handle ->
                pending.store(handle)
                org.maplibre.nativeffi.generated.ResourceProviderDecision.HANDLE
              }
          )
        )
      )
      try {
        session.completeOnDriver(map.setStyleJson(BACKGROUND_STYLE_JSON.encodeToByteArray()))
        session.completeOnDriver(
          map.updateCamera(CameraUpdate(camera = CameraOptions().copy(zoom = 3.0)))
        )
        val first = map.requestStillImage()
        session.renderUntilSettled()
        session.completeOnDriver(first)
        session.completeOnDriver(map.setStyleUrl("test://pending-style.json"))
        session.completeOnDriver(
          map.updateCamera(CameraUpdate(camera = CameraOptions().copy(zoom = 6.0)))
        )
        val second = map.requestStillImage()
        // The provider can receive the URL before the still-image request starts.
        // A later camera query observes its synchronously published render update.
        assertEquals(6.0, session.completeOnDriver(map.cameraQuery()).camera.zoom)
        val completedBeforeRender = second.isCompleted
        for (attempt in 0 until 500) {
          if (pending.load() != null) break
          sleepMillis(1)
        }
        assertNotNull(pending.load())
        val skipped = session.renderOneFrame()
        val completedAfterRender = second.isCompleted
        val waitingZoom = session.projectionCreate().use { it.getCamera().zoom }
        pending
          .load()!!
          .resourceRequestComplete(
            ResourceResponse(
              status = ResourceResponseStatus.OK,
              bytes = BACKGROUND_STYLE_JSON.encodeToByteArray(),
            )
          )
        session.renderUntilSettled()
        session.completeOnDriver(second)
        assertFalse(completedBeforeRender)
        assertFalse(completedAfterRender)
        assertEquals(RenderResult.NO_UPDATE, skipped.disposition)
        assertFalse(skipped.needsRepaint)
        assertEquals(3.0, waitingZoom)
        session.projectionCreate().use { assertEquals(6.0, it.getCamera().zoom) }
        session.completeOnDriver(session.detach())
      } finally {
        pending.load()?.close()
      }
    }
  }

  @Test
  fun renderedProjectionFollowsRenderedUpdatesAndOutlivesTheSession(): Unit = runSuspendTest {
    var retained: org.maplibre.nativeffi.map.MapProjectionHandle? = null
    try {
      withOwnedTextureSession { runtime, map, owned ->
        val session = owned.session
        assertFailsWith<org.maplibre.nativeffi.error.InvalidStateException> {
          session.projectionCreate()
        }
        session.completeOnDriver(map.setStyleJson(BACKGROUND_STYLE_JSON.encodeToByteArray()))
        session.completeOnDriver(
          map.updateCamera(CameraUpdate(camera = CameraOptions().copy(zoom = 3.0)))
        )
        session.completeOnDriver(runtime.barrier())
        session.renderUntilSettled()
        session.completeOnDriver(
          map.updateCamera(CameraUpdate(camera = CameraOptions().copy(zoom = 6.0)))
        )
        val projection = session.projectionCreate()
        retained = projection
        assertEquals(3.0, projection.getCamera().zoom)
        val frame = assertNotNull(session.acquireFrame())
        session.projectionCreate().use { assertEquals(3.0, it.getCamera().zoom) }
        frame.release()
        assertEquals(RenderResult.RENDERED, session.renderOneFrame().disposition)
        session.projectionCreate().use { assertEquals(6.0, it.getCamera().zoom) }
        session.completeOnDriver(session.resize(RenderTargetExtent(16u, 8u, 1.0)))
        assertFailsWith<org.maplibre.nativeffi.error.InvalidStateException> {
          session.projectionCreate()
        }
        session.completeOnDriver(session.detach())
      }
      retained?.let { assertEquals(3.0, it.getCamera().zoom) }
    } finally {
      retained?.close()
    }
  }

  @Test
  fun ownedTextureSessionRendersReadsBackAcquiresAFrameAndDetaches(): Unit = runSuspendTest {
    withOwnedTextureSession { runtime, map, owned ->
      val session = owned.session
      assertSame(map, session.map())
      assertEquals(RenderDriverKind.CALLER_GRAPHICS_THREAD, session.getCapabilities().driver)
      assertTrue(RenderSessionCapabilityFlag.READBACK in session.getCapabilities().flags)
      assertEquals(RenderSessionState.ATTACHED, session.getSnapshot().state)

      session.completeOnDriver(map.setStyleJson(BACKGROUND_STYLE_JSON.encodeToByteArray()))
      session.completeOnDriver(runtime.barrier())

      // A second owned texture on the same map is rejected while this one is attached.
      val secondFailure =
        runCatching {
            val second = owned.attachAnotherOwnedTexture(16, 8)
            try {
              session.completeOnDriver(second.ready)
            } finally {
              second.session.abandonAndClose()
            }
          }
          .exceptionOrNull()
      assertTrue(secondFailure is MaplibreException, "second attach must fail: $secondFailure")

      val rendered = session.renderUntilSettled()
      assertEquals(RenderResult.RENDERED, rendered.disposition)

      val readback = session.completeOnDriver(session.textureReadPremultipliedRgba8())
      assertEquals(32u, readback.info.width)
      assertEquals(16u, readback.info.height)
      assertEquals(128u, readback.info.stride)
      assertEquals(
        readback.info.stride.toULong() * readback.info.height.toULong(),
        readback.info.byteLength,
      )
      assertEquals(readback.info.byteLength.toInt(), readback.data.size)

      val frame = assertNotNull(session.acquireFrame())
      assertEquals(rendered.frameGeneration, frame.getResult().frameGeneration)
      assertEquals(OwnedTextureFrameSize(32, 16), owned.frameSize(frame))
      frame.release()
      assertTrue(frame.isReleased)

      // the scale factor is fixed at attachment, so only width and height may change.
      assertFailsWith<InvalidArgumentException> {
        session.resize(RenderTargetExtent(16u, 8u, 2.0)).await()
      }

      // Resizing hands the new logical size to the map, so the next frames report
      // SIZE_PENDING until the map publishes an update matching the new target.
      session.completeOnDriver(session.resize(RenderTargetExtent(16u, 8u, 1.0)))
      session.renderUntilSettled()
      val resized = session.getSnapshot().extent
      assertEquals(16u, resized.width)
      assertEquals(8u, resized.height)
      assertEquals(1.0, resized.scaleFactor)

      session.completeOnDriver(session.barrier())
      session.completeOnDriver(session.detach())
      assertEquals(RenderSessionState.DETACHED, session.getSnapshot().state)
      assertFalse(session.isClosed)
    }
  }

  @Test
  fun renderedFrameResultsReportNeedsRepaintDuringPaintTransition(): Unit = runSuspendTest {
    withOwnedTextureSession { runtime, map, owned ->
      val session = owned.session
      session.completeOnDriver(map.setStyleJson(BACKGROUND_STYLE_JSON.encodeToByteArray()))
      session.completeOnDriver(runtime.barrier())
      session.renderUntilSettled()

      session.completeOnDriver(
        map.setLayerProperty(
          "bg",
          "background-color-transition",
          """{"duration":60000}""".encodeToByteArray(),
        )
      )
      session.completeOnDriver(
        map.setLayerProperty("bg", "background-color", "\"#0000ff\"".encodeToByteArray())
      )
      session.completeOnDriver(runtime.barrier())

      var sawRepaintRequest = false
      for (attempt in 0 until 500) {
        val result = session.renderOneFrame()
        sleepMillis(1)
        if (result.disposition == RenderResult.RENDERED && result.needsRepaint) {
          sawRepaintRequest = true
          break
        }
      }
      assertTrue(sawRepaintRequest)

      session.completeOnDriver(session.detach())
    }
  }

  @Test
  fun aSettledMapReportsNoUpdateForAnIfNeededDemand(): Unit = runSuspendTest {
    withOwnedTextureSession { runtime, map, owned ->
      val session = owned.session
      session.completeOnDriver(map.setStyleJson(BACKGROUND_STYLE_JSON.encodeToByteArray()))
      session.completeOnDriver(runtime.barrier())
      session.renderUntilSettled()

      // The map published nothing after the settled frame, so the session skips the render.
      assertEquals(
        RenderResult.NO_UPDATE,
        session.renderOneFrame(GeneratedApi.frameDemandDefault()).disposition,
      )
      assertEquals(RenderSessionState.ATTACHED, session.getSnapshot().state)

      session.completeOnDriver(session.detach())
    }
  }

  @Test
  fun abandonReportsItsDispositionAndFailsPendingDriverWorkAsTargetLost(): Unit = runSuspendTest {
    withOwnedTextureSession(width = 8, height = 8) { runtime, map, owned ->
      val session = owned.session
      session.completeOnDriver(map.setStyleJson(BACKGROUND_STYLE_JSON.encodeToByteArray()))
      session.completeOnDriver(runtime.barrier())
      session.renderUntilSettled()

      val frame = assertNotNull(session.acquireFrame())
      val pending = session.reduceMemoryUse()
      // An attached session that rendered always leaves its renderer and target behind.
      val abandoned = session.abandon()
      assertEquals(
        org.maplibre.nativeffi.generated.RenderAbandonDisposition.QUARANTINED,
        abandoned.disposition,
      )
      assertTrue(
        abandoned.quarantinedResourceCount > 0u,
        "expected quarantined resources, got ${abandoned.quarantinedResourceCount}",
      )
      val failure = runCatching { pending.await() }.exceptionOrNull()
      assertTrue(failure is MaplibreException, "expected a target-lost failure: $failure")
      assertEquals(MaplibreStatus.TARGET_LOST, failure.status)
      frame.release()
      assertTrue(frame.isReleased)
      assertEquals(RenderSessionState.ABANDONED, session.getSnapshot().state)
    }
  }

  // set_target reports unsupported for a target kind the session does
  // not have. Dummy descriptors are enough: native checks the session kind
  // before it reads GPU objects.

  @Test
  fun ownedTextureSetTargetReportsUnsupportedForOtherTargetKinds(): Unit = runSuspendTest {
    withOwnedTextureSession(mapMode = MapMode.STATIC) { _, _, owned ->
      val session = owned.session
      assertUnsupported(session, "metal texture") {
        session.metalBorrowedTextureSetTarget(metalBorrowedTexture())
      }
      assertUnsupported(session, "vulkan texture") {
        session.vulkanBorrowedTextureSetTarget(vulkanBorrowedTexture())
      }
      assertUnsupported(session, "opengl texture") {
        session.openglBorrowedTextureSetTarget(openGLBorrowedTexture())
      }
      assertUnsupported(session, "metal surface") { session.metalSurfaceSetTarget(metalSurface()) }
      assertUnsupported(session, "vulkan surface") {
        session.vulkanSurfaceSetTarget(vulkanSurface())
      }
      assertUnsupported(session, "opengl surface") {
        session.openglSurfaceSetTarget(openGLSurface())
      }
    }
  }

  /** A target-kind mismatch fails either at submission or at completion. */
  private suspend fun assertUnsupported(
    session: RenderSessionHandle,
    label: String,
    submit: () -> Deferred<Unit>,
  ) {
    val failure = runCatching { session.completeOnDriver(submit()) }.exceptionOrNull()
    assertTrue(failure is MaplibreException, "$label: expected an unsupported failure: $failure")
    assertEquals(MaplibreStatus.UNSUPPORTED, failure.status, "$label: ${failure.diagnostic}")
  }

  private companion object {
    private const val BACKGROUND_STYLE_JSON =
      """{"version":8,"sources":{},"layers":[{"id":"bg","type":"background","paint":{"background-color":"#ff0000"}}]}"""
  }
}

private fun dummyPointer(): NativePointer = NativePointer.ofAddress(1)

private fun metalBorrowedTexture() =
  GeneratedApi.metalBorrowedTextureDescriptorDefault()
    .copy(
      extent = RenderTargetExtent(16u, 8u, 1.0),
      physicalWidth = 16u,
      physicalHeight = 8u,
      texture = dummyPointer(),
    )

private fun metalSurface() =
  GeneratedApi.metalSurfaceDescriptorDefault()
    .copy(
      extent = RenderTargetExtent(16u, 8u, 1.0),
      context = MetalContextDescriptor(dummyPointer()),
      layer = dummyPointer(),
    )

private fun vulkanContext() =
  VulkanContextDescriptor(
    instance = dummyPointer(),
    physicalDevice = dummyPointer(),
    device = dummyPointer(),
    graphicsQueue = dummyPointer(),
    graphicsQueueFamilyIndex = 0u,
    getInstanceProcAddr = dummyPointer(),
    getDeviceProcAddr = dummyPointer(),
  )

private fun vulkanBorrowedTexture() =
  GeneratedApi.vulkanBorrowedTextureDescriptorDefault()
    .copy(
      extent = RenderTargetExtent(16u, 8u, 1.0),
      physicalWidth = 16u,
      physicalHeight = 8u,
      context = vulkanContext(),
      image = 1uL,
      imageView = 1uL,
      format = 37u,
      initialLayout = 5u,
      finalLayout = 5u,
    )

private fun vulkanSurface() =
  GeneratedApi.vulkanSurfaceDescriptorDefault()
    .copy(extent = RenderTargetExtent(16u, 8u, 1.0), context = vulkanContext(), surface = 1uL)

private fun eglContext() =
  OpenglContextDescriptor(
    data =
      OpenglContextDescriptorData.Egl(
        EglContextDescriptor(
          display = dummyPointer(),
          config = dummyPointer(),
          shareContext = dummyPointer(),
          getProcAddress = NativePointer.NULL_POINTER,
        )
      )
  )

private fun openGLBorrowedTexture() =
  GeneratedApi.openglBorrowedTextureDescriptorDefault()
    .copy(
      extent = RenderTargetExtent(16u, 8u, 1.0),
      physicalWidth = 16u,
      physicalHeight = 8u,
      context = eglContext(),
      texture = 1u,
      target = 0x0DE1u,
    )

private fun openGLSurface() =
  GeneratedApi.openglSurfaceDescriptorDefault()
    .copy(
      extent = RenderTargetExtent(16u, 8u, 1.0),
      context = eglContext(),
      surface = dummyPointer(),
    )
