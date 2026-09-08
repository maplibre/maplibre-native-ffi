package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertSame
import kotlin.test.assertTrue
import kotlinx.cinterop.BetaInteropApi
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.ObjCObject
import kotlinx.cinterop.addressOf
import kotlinx.cinterop.objcPtr
import kotlinx.cinterop.toLong
import kotlinx.cinterop.useContents
import kotlinx.cinterop.usePinned
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.map.MapSize
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions
import org.maplibre.nativeffi.runtime.runSuspendTest
import platform.CoreGraphics.CGSizeMake
import platform.Metal.MTLCreateSystemDefaultDevice
import platform.Metal.MTLDeviceProtocol
import platform.Metal.MTLPixelFormatBGRA8Unorm
import platform.Metal.MTLPixelFormatRGBA8Unorm
import platform.Metal.MTLRegionMake2D
import platform.Metal.MTLTextureDescriptor
import platform.Metal.MTLTextureProtocol
import platform.Metal.MTLTextureUsageRenderTarget
import platform.Metal.MTLTextureUsageShaderRead
import platform.QuartzCore.CAMetalLayer

@OptIn(BetaInteropApi::class, ExperimentalForeignApi::class)
class MetalRenderTargetTest {
  // BND-162, BND-171, BND-175, BND-176: borrowed-texture and surface attach paths preserve
  // caller-owned backend handles, accept a replacement target, and reject each other's kind.
  //
  // These sessions run on the core-worker driver, so a test demands frames and polls the
  // frame-result queue instead of servicing driver work.

  @Test
  fun metalBorrowedTextureAndSurfaceAttachThroughPublicBinding(): Unit = runSuspendTest {
    if (!metalSupportedOrInapplicable()) return@runSuspendTest
    val device =
      MTLCreateSystemDefaultDevice() ?: error("MTLCreateSystemDefaultDevice returned nil")
    val runtime = RuntimeHandle.create(RuntimeOptions())
    try {
      val borrowedTexture = createMetalTexture(device, 32, 16)
      val borrowedMap = createMap(runtime)
      try {
        val borrowedTextureAddress = borrowedTexture.address()
        val borrowedDescriptor =
          MetalBorrowedTextureDescriptor(
            extent = RenderTargetExtent(32, 16, 1.0),
            physicalWidth = 32,
            physicalHeight = 16,
            texture = NativePointer.ofAddress(borrowedTextureAddress),
          )
        val attachment = borrowedMap.attachMetalBorrowedTexture(borrowedDescriptor)
        val session = attachment.session
        try {
          attachment.completed.await()
          assertSame(borrowedMap, session.map())
          borrowedMap.setStyleJson(QUERY_STYLE_JSON.encodeToByteArray()).await()
          runtime.barrier().await()
          assertEquals(RenderResult.RENDERED, session.awaitRenderedFrame().disposition)

          // A borrowed target has no texture ring and no readback path.
          assertFalse(session.capabilities().frameAcquisition)
          assertFalse(session.capabilities().readback)
          assertUnsupported { session.readPremultipliedRgba8() }
        } finally {
          session.abandonAndClose()
          runtime.barrier().await()
        }
        // The session never retained the texture the host attached with.
        assertEquals(borrowedTextureAddress, borrowedDescriptor.texture.address)
      } finally {
        borrowedMap.close()
        runtime.barrier().await()
      }

      val layer = createMetalLayer(device, 32, 16)
      val surfaceMap = createMap(runtime)
      try {
        val attachment =
          surfaceMap.attachMetalSurface(
            MetalSurfaceDescriptor(
              extent = RenderTargetExtent(32, 16, 1.0),
              context = MetalContextDescriptor(NativePointer.ofAddress(device.address())),
              layer = NativePointer.ofAddress(layer.address()),
            )
          )
        val session = attachment.session
        try {
          attachment.completed.await()
          assertSame(surfaceMap, session.map())
          surfaceMap.setStyleJson(QUERY_STYLE_JSON.encodeToByteArray()).await()
          runtime.barrier().await()
          assertEquals(RenderResult.RENDERED, session.awaitRenderedFrame().disposition)

          assertFalse(session.capabilities().frameAcquisition)
          assertFalse(session.capabilities().readback)
          assertUnsupported { session.readPremultipliedRgba8() }
        } finally {
          session.abandonAndClose()
          runtime.barrier().await()
        }
      } finally {
        surfaceMap.close()
        runtime.barrier().await()
      }
    } finally {
      runtime.close().await()
    }
  }

  // A null device at attach uses MTLCreateSystemDefaultDevice(), and the
  // session writes drawableSize from the extent's physical size.

  @Test
  fun metalSurfaceAttachAcceptsNullDeviceAndWritesDrawableSize(): Unit = runSuspendTest {
    if (!metalSupportedOrInapplicable()) return@runSuspendTest
    val runtime = RuntimeHandle.create(RuntimeOptions())
    try {
      val map = createMap(runtime)
      try {
        val layer = CAMetalLayer()
        layer.drawableSize = CGSizeMake(1.0, 1.0)
        val extent = RenderTargetExtent(32, 16, 2.0)
        val physical = extent.physicalSize()
        val attachment =
          map.attachMetalSurface(
            MetalSurfaceDescriptor(
              extent = extent,
              context = MetalContextDescriptor(NativePointer.NULL_POINTER),
              layer = NativePointer.ofAddress(layer.address()),
            )
          )
        val session = attachment.session
        try {
          attachment.completed.await()
          layer.drawableSize.useContents {
            assertEquals(physical.width.toDouble(), width)
            assertEquals(physical.height.toDouble(), height)
          }
          map.setStyleJson(QUERY_STYLE_JSON.encodeToByteArray()).await()
          runtime.barrier().await()
          assertEquals(RenderResult.RENDERED, session.awaitRenderedFrame().disposition)
        } finally {
          session.abandonAndClose()
          runtime.barrier().await()
        }
      } finally {
        map.close()
        runtime.barrier().await()
      }
    } finally {
      runtime.close().await()
    }
  }

  // Replacing a host-owned target keeps the session and hands it the replacement's extent,
  // which the map catches up to on a later frame.

  @Test
  fun metalSetTargetReplacesBorrowedTextureAndSurfaceTargets(): Unit = runSuspendTest {
    if (!metalSupportedOrInapplicable()) return@runSuspendTest
    val device =
      MTLCreateSystemDefaultDevice() ?: error("MTLCreateSystemDefaultDevice returned nil")
    val runtime = RuntimeHandle.create(RuntimeOptions())
    try {
      val borrowedTexture = createMetalTexture(device, 32, 16)
      val borrowedMap = createMap(runtime)
      try {
        val borrowedTextureAddress = borrowedTexture.address()
        val attachment =
          borrowedMap.attachMetalBorrowedTexture(
            MetalBorrowedTextureDescriptor(
              extent = RenderTargetExtent(32, 16, 1.0),
              physicalWidth = 32,
              physicalHeight = 16,
              texture = NativePointer.ofAddress(borrowedTextureAddress),
            )
          )
        val session = attachment.session
        try {
          attachment.completed.await()
          borrowedMap.setStyleJson(QUERY_STYLE_JSON.encodeToByteArray()).await()
          runtime.barrier().await()
          assertEquals(RenderResult.RENDERED, session.awaitRenderedFrame().disposition)

          // A caller-owned texture is sized by its owner, so the host reallocates it and hands the
          // replacement over. The map remains the independent logical-size authority.
          assertUnsupported { session.resize(RenderTargetExtent(16, 8, 1.0)) }
          val replacementTexture = createMetalTexture(device, 16, 8)
          val replacement =
            MetalBorrowedTextureDescriptor(
              extent = RenderTargetExtent(16, 8, 1.0),
              physicalWidth = 16,
              physicalHeight = 8,
              texture = NativePointer.ofAddress(replacementTexture.address()),
            )
          // Freshly allocated, so anything non-zero later came from this
          // session rendering into it after the handoff.
          assertTrue(
            readMetalTextureRgba(device, replacementTexture, 16, 8).all { it == 0.toByte() },
            "a freshly allocated replacement should start blank",
          )
          session.setMetalBorrowedTextureTarget(replacement).await()
          borrowedMap.resize(MapSize(16, 8, 1.0)).await()
          runtime.barrier().await()
          assertSame(borrowedMap, session.map())
          assertEquals(RenderResult.RENDERED, session.awaitRenderedFrame().disposition)
          // The session paints the texture it was handed, not the one it had.
          assertTrue(
            readMetalTextureRgba(device, replacementTexture, 16, 8).any { it != 0.toByte() },
            "the replacement texture should have been rendered into",
          )
          assertEquals(replacementTexture.address(), replacement.texture.address)
          // The session never retained the outgoing texture, so the host
          // still owns the one it attached with.
          assertEquals(borrowedTextureAddress, borrowedTexture.address())
        } finally {
          session.abandonAndClose()
          runtime.barrier().await()
        }
      } finally {
        borrowedMap.close()
        runtime.barrier().await()
      }

      val layer = createMetalLayer(device, 32, 16)
      val replacementLayer = createMetalLayer(device, 16, 8)
      val surfaceMap = createMap(runtime)
      try {
        val attachment =
          surfaceMap.attachMetalSurface(
            MetalSurfaceDescriptor(
              extent = RenderTargetExtent(32, 16, 1.0),
              context = MetalContextDescriptor(NativePointer.ofAddress(device.address())),
              layer = NativePointer.ofAddress(layer.address()),
            )
          )
        val session = attachment.session
        try {
          attachment.completed.await()
          surfaceMap.setStyleJson(QUERY_STYLE_JSON.encodeToByteArray()).await()
          runtime.barrier().await()
          assertEquals(RenderResult.RENDERED, session.awaitRenderedFrame().disposition)

          // A recreated host surface replaces the presentation target while
          // the session and its renderer go on living.
          session
            .setMetalSurfaceTarget(
              MetalSurfaceDescriptor(
                extent = RenderTargetExtent(16, 8, 1.0),
                context = MetalContextDescriptor(NativePointer.ofAddress(device.address())),
                layer = NativePointer.ofAddress(replacementLayer.address()),
              )
            )
            .await()
          surfaceMap.resize(MapSize(16, 8, 1.0)).await()
          runtime.barrier().await()
          assertSame(surfaceMap, session.map())
          assertEquals(RenderResult.RENDERED, session.awaitRenderedFrame().disposition)
        } finally {
          session.abandonAndClose()
          runtime.barrier().await()
        }
      } finally {
        surfaceMap.close()
        runtime.barrier().await()
      }
    } finally {
      runtime.close().await()
    }
  }

  // A borrowed texture session and a surface session reject each other's set-target pairing.

  @Test
  fun metalSetTargetReportsUnsupportedForSwappedBorrowedAndSurfaceTargets(): Unit = runSuspendTest {
    if (!metalSupportedOrInapplicable()) return@runSuspendTest
    val device =
      MTLCreateSystemDefaultDevice() ?: error("MTLCreateSystemDefaultDevice returned nil")
    val metalContext = MetalContextDescriptor(NativePointer.ofAddress(device.address()))
    val runtime = RuntimeHandle.create(RuntimeOptions())
    try {
      val texture = createMetalTexture(device, 32, 16)
      val textureTarget =
        MetalBorrowedTextureDescriptor(
          extent = RenderTargetExtent(32, 16, 1.0),
          physicalWidth = 32,
          physicalHeight = 16,
          texture = NativePointer.ofAddress(texture.address()),
        )
      val layer = createMetalLayer(device, 32, 16)
      val surfaceTarget =
        MetalSurfaceDescriptor(
          extent = RenderTargetExtent(32, 16, 1.0),
          context = metalContext,
          layer = NativePointer.ofAddress(layer.address()),
        )

      val borrowedMap = createMap(runtime)
      try {
        val attachment = borrowedMap.attachMetalBorrowedTexture(textureTarget)
        try {
          attachment.completed.await()
          assertUnsupported { attachment.session.setMetalSurfaceTarget(surfaceTarget) }
        } finally {
          attachment.session.abandonAndClose()
          runtime.barrier().await()
        }
      } finally {
        borrowedMap.close()
        runtime.barrier().await()
      }

      val surfaceMap = createMap(runtime)
      try {
        val attachment = surfaceMap.attachMetalSurface(surfaceTarget)
        try {
          attachment.completed.await()
          assertUnsupported { attachment.session.setMetalBorrowedTextureTarget(textureTarget) }
        } finally {
          attachment.session.abandonAndClose()
          runtime.barrier().await()
        }
      } finally {
        surfaceMap.close()
        runtime.barrier().await()
      }
    } finally {
      runtime.close().await()
    }
  }

  /** A target-kind mismatch fails either at submission or at completion. */
  private suspend fun assertUnsupported(submit: () -> Deferred<*>) {
    val failure = runCatching { submit().await() }.exceptionOrNull()
    assertTrue(failure is MaplibreException, "expected an unsupported failure: $failure")
    assertEquals(MaplibreStatus.UNSUPPORTED, failure.status, failure.diagnostic)
  }

  private fun metalSupportedOrInapplicable(): Boolean {
    return RenderBackend.METAL in Maplibre.supportedRenderBackends()
  }

  private suspend fun createMap(runtime: RuntimeHandle): MapHandle =
    MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 32
          height = 16
        },
      )
      .await()

  private fun createMetalTexture(device: MTLDeviceProtocol, width: Int, height: Int): ObjCObject {
    val descriptor =
      MTLTextureDescriptor.texture2DDescriptorWithPixelFormat(
        MTLPixelFormatRGBA8Unorm,
        width.toULong(),
        height.toULong(),
        false,
      )
    descriptor.usage = MTLTextureUsageRenderTarget or MTLTextureUsageShaderRead
    val texture =
      device.newTextureWithDescriptor(descriptor) ?: error("Metal texture creation failed")
    // A new MTLTexture's contents are undefined, so clear it: a test that reads
    // this back to prove a session rendered into it needs a known start value.
    val blank = ByteArray(width * height * 4)
    blank.usePinned { pinned ->
      texture.replaceRegion(
        MTLRegionMake2D(0u, 0u, width.toULong(), height.toULong()),
        0u,
        pinned.addressOf(0),
        (width * 4).toULong(),
      )
    }
    return texture
  }

  // Reads a borrowed texture back to the CPU. A texture created without an
  // explicit storage mode is managed on macOS, so its CPU copy stays stale
  // until a blit synchronizes it.
  private fun readMetalTextureRgba(
    device: MTLDeviceProtocol,
    texture: ObjCObject,
    width: Int,
    height: Int,
  ): ByteArray {
    val metalTexture = texture as MTLTextureProtocol
    val queue = device.newCommandQueue() ?: error("MTLDevice.newCommandQueue returned nil")
    val commandBuffer = queue.commandBuffer() ?: error("MTLCommandQueue.commandBuffer returned nil")
    val encoder =
      commandBuffer.blitCommandEncoder()
        ?: error("MTLCommandBuffer.blitCommandEncoder returned nil")
    encoder.synchronizeTextureForCpu(metalTexture)
    encoder.endEncoding()
    commandBuffer.commit()
    commandBuffer.waitUntilCompleted()

    val pixels = ByteArray(width * height * 4)
    pixels.usePinned { pinned ->
      metalTexture.getBytes(
        pinned.addressOf(0),
        (width * 4).toULong(),
        MTLRegionMake2D(0u, 0u, width.toULong(), height.toULong()),
        0u,
      )
    }
    return pixels
  }

  private fun createMetalLayer(device: MTLDeviceProtocol, width: Int, height: Int): CAMetalLayer {
    val layer = CAMetalLayer()
    layer.device = device as objcnames.protocols.MTLDeviceProtocol
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm
    layer.framebufferOnly = true
    layer.drawableSize = CGSizeMake(width.toDouble(), height.toDouble())
    return layer
  }

  private fun ObjCObject.address(): Long = objcPtr().toLong()
}
