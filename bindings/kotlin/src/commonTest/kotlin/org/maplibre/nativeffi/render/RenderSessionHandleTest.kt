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
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.CameraUpdate
import org.maplibre.nativeffi.camera.CameraUpdateMode
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.runtime.runSuspendTest
import org.maplibre.nativeffi.sleepMillis

class RenderSessionHandleTest {
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
        return session.completeOnDriver(session.readPremultipliedRgba8()).bytes.take(4).map {
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
          org.maplibre.nativeffi.resource.ResourceProviderCallback { _, handle ->
            pending.store(handle)
            org.maplibre.nativeffi.resource.ResourceProviderDecision.HANDLE
          }
        )
      )
      try {
        session.completeOnDriver(map.setStyleJson(BACKGROUND_STYLE_JSON.encodeToByteArray()))
        session.completeOnDriver(
          map.updateCamera(CameraUpdate(camera = CameraOptions().apply { zoom = 3.0 }))
        )
        val first = map.requestStillImage()
        session.renderUntilSettled()
        session.completeOnDriver(first)
        session.completeOnDriver(map.setStyleUrl("test://pending-style.json"))
        session.completeOnDriver(
          map.updateCamera(CameraUpdate(camera = CameraOptions().apply { zoom = 6.0 }))
        )
        val second = map.requestStillImage()
        for (attempt in 0 until 500) {
          if (pending.load() != null) break
          sleepMillis(1)
        }
        assertNotNull(pending.load())
        val skipped = session.renderOneFrame()
        assertEquals(RenderResult.NO_UPDATE, skipped.disposition)
        assertFalse(skipped.needsRepaint)
        session.createProjection().use { assertEquals(3.0, it.camera().zoom) }
        pending
          .load()!!
          .complete(
            org.maplibre.nativeffi.resource
              .ResourceResponse(org.maplibre.nativeffi.resource.ResourceResponseStatus.OK)
              .apply { bytes = BACKGROUND_STYLE_JSON.encodeToByteArray() }
          )
        session.renderUntilSettled()
        session.completeOnDriver(second)
        session.createProjection().use { assertEquals(6.0, it.camera().zoom) }
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
          session.createProjection()
        }
        session.completeOnDriver(map.setStyleJson(BACKGROUND_STYLE_JSON.encodeToByteArray()))
        session.completeOnDriver(
          map.updateCamera(CameraUpdate(camera = CameraOptions().apply { zoom = 3.0 }))
        )
        session.completeOnDriver(runtime.barrier())
        session.renderUntilSettled()
        session.completeOnDriver(
          map.updateCamera(CameraUpdate(camera = CameraOptions().apply { zoom = 6.0 }))
        )
        val projection = session.createProjection()
        retained = projection
        assertEquals(3.0, projection.camera().zoom)
        val frame = assertNotNull(session.acquireFrame())
        session.createProjection().use { assertEquals(3.0, it.camera().zoom) }
        frame.release()
        assertEquals(RenderResult.RENDERED, session.renderOneFrame().disposition)
        session.createProjection().use { assertEquals(6.0, it.camera().zoom) }
        session.completeOnDriver(session.resize(RenderTargetExtent(16, 8, 1.0)))
        assertFailsWith<org.maplibre.nativeffi.error.InvalidStateException> {
          session.createProjection()
        }
        session.completeOnDriver(session.detach())
      }
      retained?.let { assertEquals(3.0, it.camera().zoom) }
    } finally {
      retained?.close()
    }
  }

  @Test
  fun ownedTextureSessionRendersReadsBackAcquiresAFrameAndDetaches(): Unit = runSuspendTest {
    withOwnedTextureSession { runtime, map, owned ->
      val session = owned.session
      assertSame(map, session.map())
      assertEquals(RenderDriver.CALLER_GRAPHICS_THREAD, session.capabilities().driver)
      assertTrue(session.capabilities().readback)
      assertEquals(RenderSessionState.ATTACHED, session.snapshot().state)

      session.completeOnDriver(map.setStyleJson(BACKGROUND_STYLE_JSON.encodeToByteArray()))
      session.completeOnDriver(runtime.barrier())

      // A second owned texture on the same map is rejected while this one is attached.
      val secondFailure =
        runCatching {
            val second = owned.attachAnotherOwnedTexture(16, 8)
            try {
              session.completeOnDriver(second.completed)
            } finally {
              second.session.abandonAndClose()
            }
          }
          .exceptionOrNull()
      assertTrue(secondFailure is MaplibreException, "second attach must fail: $secondFailure")

      val rendered = session.renderUntilSettled()
      assertEquals(RenderResult.RENDERED, rendered.disposition)

      val readback = session.completeOnDriver(session.readPremultipliedRgba8())
      assertEquals(32, readback.info.width)
      assertEquals(16, readback.info.height)
      assertEquals(32 * 4, readback.info.stride)
      assertEquals(
        readback.info.stride.toLong() * readback.info.height.toLong(),
        readback.info.byteLength,
      )
      assertEquals(readback.info.byteLength.toInt(), readback.bytes.size)

      val frame = assertNotNull(session.acquireFrame())
      assertEquals(rendered.frameGeneration, frame.result().frameGeneration)
      assertEquals(OwnedTextureFrameSize(32, 16), owned.frameSize(frame))
      frame.release()
      assertTrue(frame.isReleased)

      // BND-183: the scale factor is fixed at attachment, so only width and height may change.
      assertFailsWith<InvalidArgumentException> {
        session.resize(RenderTargetExtent(16, 8, 2.0)).await()
      }

      // Resizing hands the new logical size to the map, so the next frames report
      // SIZE_PENDING until the map publishes an update matching the new target.
      session.completeOnDriver(session.resize(RenderTargetExtent(16, 8, 1.0)))
      session.renderUntilSettled()
      val resized = session.snapshot().extent
      assertEquals(16, resized.width)
      assertEquals(8, resized.height)
      assertEquals(1.0, resized.scaleFactor)

      session.completeOnDriver(session.barrier())
      session.completeOnDriver(session.detach())
      assertEquals(RenderSessionState.DETACHED, session.snapshot().state)
      assertFalse(session.isClosed)
    }
  }

  @Test
  fun renderedFrameResultsReportNeedsRepaintDuringCameraTransition(): Unit = runSuspendTest {
    withOwnedTextureSession { runtime, map, owned ->
      val session = owned.session
      session.completeOnDriver(map.setStyleJson(BACKGROUND_STYLE_JSON.encodeToByteArray()))
      session.completeOnDriver(runtime.barrier())
      session.renderUntilSettled()

      session.completeOnDriver(
        map.updateCamera(
          CameraUpdate(
            mode = CameraUpdateMode.EASE,
            camera = CameraOptions().apply { zoom = 4.0 },
            animation = AnimationOptions().apply { durationMs = 60_000.0 },
          )
        )
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
      assertEquals(RenderResult.NO_UPDATE, session.renderOneFrame(FrameDemand()).disposition)
      assertEquals(RenderSessionState.ATTACHED, session.snapshot().state)

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
      assertEquals(RenderAbandonDisposition.QUARANTINED, abandoned.disposition)
      assertTrue(
        abandoned.quarantinedResourceCount > 0,
        "expected quarantined resources, got ${abandoned.quarantinedResourceCount}",
      )
      val failure = runCatching { pending.await() }.exceptionOrNull()
      assertTrue(failure is MaplibreException, "expected a target-lost failure: $failure")
      assertEquals(MaplibreStatus.TARGET_LOST, failure.status)
      frame.release()
      assertTrue(frame.isReleased)
      assertEquals(RenderSessionState.ABANDONED, session.snapshot().state)
    }
  }

  // BND-176: set_target reports unsupported for a target kind the session does
  // not have. Dummy descriptors are enough: native checks the session kind
  // before it reads GPU objects.

  @Test
  fun ownedTextureSetTargetReportsUnsupportedForOtherTargetKinds(): Unit = runSuspendTest {
    withOwnedTextureSession(mapMode = MapMode.STATIC) { _, _, owned ->
      val session = owned.session
      assertUnsupported(session, "metal texture") {
        session.setMetalBorrowedTextureTarget(metalBorrowedTexture())
      }
      assertUnsupported(session, "vulkan texture") {
        session.setVulkanBorrowedTextureTarget(vulkanBorrowedTexture())
      }
      assertUnsupported(session, "opengl texture") {
        session.setOpenGLBorrowedTextureTarget(openGLBorrowedTexture())
      }
      assertUnsupported(session, "metal surface") { session.setMetalSurfaceTarget(metalSurface()) }
      assertUnsupported(session, "vulkan surface") {
        session.setVulkanSurfaceTarget(vulkanSurface())
      }
      assertUnsupported(session, "opengl surface") {
        session.setOpenGLSurfaceTarget(openGLSurface())
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

private fun dummyVulkanHandle(): VulkanHandle = VulkanHandle.ofBits(1)

private fun metalBorrowedTexture(): MetalBorrowedTextureDescriptor =
  MetalBorrowedTextureDescriptor(RenderTargetExtent(16, 8, 1.0), 16, 8, dummyPointer())

private fun metalSurface(): MetalSurfaceDescriptor =
  MetalSurfaceDescriptor(
    RenderTargetExtent(16, 8, 1.0),
    MetalContextDescriptor(dummyPointer()),
    dummyPointer(),
  )

private fun vulkanContext(): VulkanContextDescriptor =
  VulkanContextDescriptor(
    dummyPointer(),
    dummyPointer(),
    dummyPointer(),
    dummyPointer(),
    0,
    dummyPointer(),
    dummyPointer(),
  )

private fun vulkanBorrowedTexture(): VulkanBorrowedTextureDescriptor =
  VulkanBorrowedTextureDescriptor(
    RenderTargetExtent(16, 8, 1.0),
    16,
    8,
    vulkanContext(),
    dummyVulkanHandle(),
    dummyVulkanHandle(),
    // VK_FORMAT_R8G8B8A8_UNORM and VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
    37,
    5,
  )

private fun vulkanSurface(): VulkanSurfaceDescriptor =
  VulkanSurfaceDescriptor(RenderTargetExtent(16, 8, 1.0), vulkanContext(), dummyVulkanHandle())

private fun eglContext(): EglContextDescriptor =
  EglContextDescriptor(dummyPointer(), dummyPointer(), dummyPointer(), NativePointer.NULL_POINTER)

private fun openGLBorrowedTexture(): OpenGLBorrowedTextureDescriptor =
  OpenGLBorrowedTextureDescriptor(RenderTargetExtent(16, 8, 1.0), 16, 8, eglContext(), 1, 0x0DE1)

private fun openGLSurface(): OpenGLSurfaceDescriptor =
  OpenGLSurfaceDescriptor(RenderTargetExtent(16, 8, 1.0), eglContext(), dummyPointer())
