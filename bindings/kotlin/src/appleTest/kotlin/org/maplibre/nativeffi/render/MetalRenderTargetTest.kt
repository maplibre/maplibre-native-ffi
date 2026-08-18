package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertSame
import kotlin.test.assertTrue
import kotlinx.cinterop.BetaInteropApi
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.ObjCObject
import kotlinx.cinterop.addressOf
import kotlinx.cinterop.objcPtr
import kotlinx.cinterop.toLong
import kotlinx.cinterop.usePinned
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.error.UnsupportedFeatureException
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
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
  // BND-162, BND-171: borrowed texture and surface attach paths preserve caller-owned backend
  // handles.

  @Test
  fun metalBorrowedTextureAndSurfaceAttachThroughPublicBinding() {
    if (!metalSupportedOrInapplicable()) return
    val device =
      MTLCreateSystemDefaultDevice() ?: error("MTLCreateSystemDefaultDevice returned nil")
    Maplibre.setLogCallback(LogCallback { true })
    Maplibre.setAsyncLogSeverities(emptySet())
    try {
      val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
      try {
        val borrowedTexture = createMetalTexture(device, 32, 16)
        val borrowedMap =
          MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
            },
          )
        try {
          val borrowedTextureAddress = borrowedTexture.address()
          val borrowedDescriptor =
            MetalBorrowedTextureDescriptor(
              extent = RenderTargetExtent(32, 16, 1.0),
              physicalWidth = 32,
              physicalHeight = 16,
              texture = NativePointer.ofAddress(borrowedTextureAddress),
            )
          val session = borrowedMap.attachMetalBorrowedTexture(borrowedDescriptor)
          try {
            assertSame(borrowedMap, session.map())
            borrowedMap.setStyleJson(QUERY_STYLE_JSON.encodeToByteArray())
            assertTrue(
              waitForMapEvent(runtime, borrowedMap, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE)
            )
            assertEquals(RenderResult.RENDERED, session.renderUpdate().result)
            assertFailsWith<UnsupportedFeatureException> { session.acquireMetalOwnedTextureFrame() }
            assertFailsWith<UnsupportedFeatureException> { session.textureImageInfo() }
          } finally {
            session.close()
          }
          assertEquals(borrowedTextureAddress, borrowedDescriptor.texture.address)
        } finally {
          borrowedMap.close()
        }

        val layer = createMetalLayer(device, 32, 16)
        val surfaceMap =
          MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
            },
          )
        try {
          val session =
            surfaceMap.attachMetalSurface(
              MetalSurfaceDescriptor(
                extent = RenderTargetExtent(32, 16, 1.0),
                context = MetalContextDescriptor(NativePointer.ofAddress(device.address())),
                layer = NativePointer.ofAddress(layer.address()),
              )
            )
          try {
            assertSame(surfaceMap, session.map())
            surfaceMap.setStyleJson(QUERY_STYLE_JSON.encodeToByteArray())
            assertTrue(
              waitForMapEvent(runtime, surfaceMap, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE)
            )
            assertEquals(RenderResult.RENDERED, session.renderUpdate().result)
            assertFailsWith<UnsupportedFeatureException> { session.acquireMetalOwnedTextureFrame() }
            assertFailsWith<UnsupportedFeatureException> { session.textureImageInfo() }
          } finally {
            session.close()
          }
        } finally {
          surfaceMap.close()
        }
      } finally {
        runtime.close()
      }
    } finally {
      Maplibre.clearLogCallback()
      Maplibre.restoreDefaultAsyncLogSeverities()
    }
  }

  // BND-175: replacing a host-owned target keeps the session and hands it the
  // replacement's extent, which the map catches up to on its next pump.

  @Test
  fun metalSetTargetReplacesBorrowedTextureAndSurfaceTargets() {
    if (!metalSupportedOrInapplicable()) return
    val device =
      MTLCreateSystemDefaultDevice() ?: error("MTLCreateSystemDefaultDevice returned nil")
    Maplibre.setLogCallback(LogCallback { true })
    Maplibre.setAsyncLogSeverities(emptySet())
    try {
      val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
      try {
        val borrowedTexture = createMetalTexture(device, 32, 16)
        val borrowedMap =
          MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
            },
          )
        try {
          val borrowedTextureAddress = borrowedTexture.address()
          val session =
            borrowedMap.attachMetalBorrowedTexture(
              MetalBorrowedTextureDescriptor(
                extent = RenderTargetExtent(32, 16, 1.0),
                physicalWidth = 32,
                physicalHeight = 16,
                texture = NativePointer.ofAddress(borrowedTextureAddress),
              )
            )
          try {
            borrowedMap.setStyleJson(QUERY_STYLE_JSON.encodeToByteArray())
            assertTrue(
              waitForMapEvent(runtime, borrowedMap, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE)
            )
            assertEquals(RenderResult.RENDERED, session.renderUpdate().result)

            // A caller-owned texture is sized by its owner, so the host
            // reallocates and hands the replacement over instead of resizing.
            assertFailsWith<UnsupportedFeatureException> { session.resize(16, 8, 1.0) }
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
            session.setMetalBorrowedTextureTarget(replacement)
            assertSame(borrowedMap, session.map())
            assertEquals(RenderResult.SIZE_PENDING, session.renderUpdate().result)
            runtime.pump(0)
            assertEquals(RenderResult.RENDERED, session.renderUpdate().result)
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
            session.close()
          }
        } finally {
          borrowedMap.close()
        }

        val layer = createMetalLayer(device, 32, 16)
        val replacementLayer = createMetalLayer(device, 16, 8)
        val surfaceMap =
          MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
            },
          )
        try {
          val session =
            surfaceMap.attachMetalSurface(
              MetalSurfaceDescriptor(
                extent = RenderTargetExtent(32, 16, 1.0),
                context = MetalContextDescriptor(NativePointer.ofAddress(device.address())),
                layer = NativePointer.ofAddress(layer.address()),
              )
            )
          try {
            surfaceMap.setStyleJson(QUERY_STYLE_JSON.encodeToByteArray())
            assertTrue(
              waitForMapEvent(runtime, surfaceMap, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE)
            )
            assertEquals(RenderResult.RENDERED, session.renderUpdate().result)

            // A recreated host surface replaces the presentation target while
            // the session and its renderer go on living.
            session.setMetalSurfaceTarget(
              MetalSurfaceDescriptor(
                extent = RenderTargetExtent(16, 8, 1.0),
                context = MetalContextDescriptor(NativePointer.ofAddress(device.address())),
                layer = NativePointer.ofAddress(replacementLayer.address()),
              )
            )
            assertSame(surfaceMap, session.map())
            assertEquals(RenderResult.SIZE_PENDING, session.renderUpdate().result)
            runtime.pump(0)
            assertEquals(RenderResult.RENDERED, session.renderUpdate().result)
          } finally {
            session.close()
          }
        } finally {
          surfaceMap.close()
        }
      } finally {
        runtime.close()
      }
    } finally {
      Maplibre.clearLogCallback()
      Maplibre.restoreDefaultAsyncLogSeverities()
    }
  }

  // BND-176: a borrowed texture session and a surface session reject each
  // other's set-target pairing.

  @Test
  fun metalSetTargetReportsUnsupportedForSwappedBorrowedAndSurfaceTargets() {
    if (!metalSupportedOrInapplicable()) return
    val device =
      MTLCreateSystemDefaultDevice() ?: error("MTLCreateSystemDefaultDevice returned nil")
    val metalContext = MetalContextDescriptor(NativePointer.ofAddress(device.address()))
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
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

      val borrowedMap = createStaticMap(runtime)
      try {
        val session = borrowedMap.attachMetalBorrowedTexture(textureTarget)
        try {
          assertFailsWith<UnsupportedFeatureException> {
            session.setMetalSurfaceTarget(surfaceTarget)
          }
        } finally {
          session.close()
        }
      } finally {
        borrowedMap.close()
      }

      val surfaceMap = createStaticMap(runtime)
      try {
        val session = surfaceMap.attachMetalSurface(surfaceTarget)
        try {
          assertFailsWith<UnsupportedFeatureException> {
            session.setMetalBorrowedTextureTarget(textureTarget)
          }
        } finally {
          session.close()
        }
      } finally {
        surfaceMap.close()
      }
    } finally {
      runtime.close()
    }
  }

  private fun metalSupportedOrInapplicable(): Boolean {
    return RenderBackend.METAL in Maplibre.supportedRenderBackends()
  }

  private fun createStaticMap(runtime: RuntimeHandle): MapHandle =
    MapHandle.create(
      runtime,
      MapOptions().apply {
        width = 64
        height = 64
        mapMode = MapMode.STATIC
      },
    )

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
