package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNotNull
import kotlin.test.assertSame
import kotlin.test.assertTrue
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.CameraUpdate
import org.maplibre.nativeffi.camera.CameraUpdateMode
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.runtime.runSuspendTest
import org.maplibre.nativeffi.sleepMillis

class RenderSessionHandleTest {
  // BND-160, BND-161, BND-163, BND-164, BND-165, BND-166, BND-167, BND-168,
  // BND-169, BND-170: owned-texture attachment, frame demands, frame leases,
  // readback, and detachment on a caller-driven graphics thread.

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

      // Resizing hands the new logical size to the map, so the next frames report
      // SIZE_PENDING until the map publishes an update matching the new target.
      session.completeOnDriver(session.resize(RenderTargetExtent(16, 8, 2.0)))
      session.renderUntilSettled()
      val resized = session.snapshot().extent
      assertEquals(16, resized.width)
      assertEquals(8, resized.height)
      assertEquals(2.0, resized.scaleFactor)

      session.completeOnDriver(session.barrier())
      session.completeOnDriver(session.detach())
      assertEquals(RenderSessionState.DETACHED, session.snapshot().state)
      assertTrue(!session.isClosed)
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
  fun abandonReportsItsDispositionAndFailsPendingDriverWorkAsTargetLost(): Unit = runSuspendTest {
    withOwnedTextureSession(width = 8, height = 8) { runtime, map, owned ->
      val session = owned.session
      session.completeOnDriver(map.setStyleJson(BACKGROUND_STYLE_JSON.encodeToByteArray()))
      session.completeOnDriver(runtime.barrier())
      session.renderUntilSettled()

      val frame = assertNotNull(session.acquireFrame())
      val pending = session.reduceMemoryUse()
      val abandoned = session.abandon()
      assertTrue(
        abandoned.disposition == RenderAbandonDisposition.CLEAN ||
          abandoned.disposition == RenderAbandonDisposition.QUARANTINED
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
