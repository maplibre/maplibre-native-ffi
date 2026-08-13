package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.UnsupportedFeatureException
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.render.MetalContextDescriptor
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.render.NativeBuffer
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.OpenGLOwnedTextureDescriptor
import org.maplibre.nativeffi.render.RenderBackend
import org.maplibre.nativeffi.render.RenderTargetExtent
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanContextDescriptor
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor
import org.maplibre.nativeffi.render.WglContextDescriptor
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

class DescriptorValidationTest {
  @Test
  fun signedCarriersRejectNegativeUnsignedValues(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      assertFailsWith<InvalidArgumentException> {
        MapOptions().apply {
          width = -1
          height = 1
        }
      }
      assertFailsWith<InvalidArgumentException> { TileOptions().prefetchZoomDelta = -1 }
      assertFailsWith<InvalidArgumentException> { NativeBuffer.allocate(-1) }
      val nullPointer = NativePointer.NULL
      assertFailsWith<InvalidArgumentException> { RenderTargetExtent(-1, 1, 1.0) }
      assertFailsWith<InvalidArgumentException> { RenderTargetExtent(1, 1, 1.0).width = -1 }
      assertFailsWith<InvalidArgumentException> {
        vulkanContext(nullPointer, graphicsQueueFamilyIndex = -1)
      }
      assertFailsWith<InvalidArgumentException> {
        vulkanContext(nullPointer).graphicsQueueFamilyIndex = -1
      }
      assertFailsWith<InvalidArgumentException> {
        vulkanBorrowedTextureDescriptor(nullPointer, format = -1)
      }
      assertFailsWith<InvalidArgumentException> {
        vulkanBorrowedTextureDescriptor(nullPointer).format = -1
      }
    }

  @Test
  fun mapAndProjectionInputsPropagateNativeCoordinateValidation(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val runtime = RuntimeHandle.create(RuntimeOptions())
      val map = MapHandle.create(runtime, mapOptions())
      var projection: MapProjectionHandle? = null
      try {
        val invalidCoordinate = LatLng(Double.NaN, 0.0)
        val createdProjection = map.createProjection()
        projection = createdProjection
        assertInvalidCoordinateDiagnostic { createdProjection.pixelForLatLng(invalidCoordinate) }
        assertInvalidCoordinateDiagnostic { Maplibre.projectedMetersForLatLng(invalidCoordinate) }
      } finally {
        projection?.close()
        map.close()
        runtime.close()
      }
    }

  @Test
  fun unsupportedRenderBackendsRejectAttachBeforeSessionCreation(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val supported = Maplibre.supportedRenderBackends()
      assertTrue(supported.isNotEmpty())
      val runtime = RuntimeHandle.create(RuntimeOptions())
      val map = MapHandle.create(runtime, mapOptions())
      try {
        val pointer = NativePointer.ofAddress(0x10L)
        val extent = RenderTargetExtent(256, 256, 1.0)
        if (RenderBackend.METAL !in supported) {
          assertEquals(
            MaplibreStatus.UNSUPPORTED,
            assertFailsWith<UnsupportedFeatureException> {
                map.attachMetalOwnedTexture(
                  MetalOwnedTextureDescriptor(extent, MetalContextDescriptor(pointer))
                )
              }
              .status,
          )
        }
        if (RenderBackend.VULKAN !in supported) {
          assertEquals(
            MaplibreStatus.UNSUPPORTED,
            assertFailsWith<UnsupportedFeatureException> {
                map.attachVulkanOwnedTexture(
                  VulkanOwnedTextureDescriptor(extent, context = vulkanContext(pointer))
                )
              }
              .status,
          )
        }
        if (RenderBackend.OPENGL !in supported) {
          assertEquals(
            MaplibreStatus.UNSUPPORTED,
            assertFailsWith<UnsupportedFeatureException> {
                map.attachOpenGLOwnedTexture(
                  OpenGLOwnedTextureDescriptor(
                    extent,
                    context = WglContextDescriptor(pointer, pointer, pointer),
                  )
                )
              }
              .status,
          )
        }
      } finally {
        map.close()
        runtime.close()
      }
    }

  private fun mapOptions(): MapOptions =
    MapOptions().apply {
      width = 128
      height = 128
    }

  private suspend fun assertInvalidCoordinateDiagnostic(block: suspend () -> Unit) {
    val error =
      try {
        block()
        error("expected InvalidArgumentException")
      } catch (error: InvalidArgumentException) {
        error
      }
    assertEquals(MaplibreStatus.INVALID_ARGUMENT, error.status)
    assertTrue(error.diagnostic.contains("latitude must be finite"))
  }

  private fun vulkanContext(
    pointer: NativePointer,
    graphicsQueueFamilyIndex: Int = 0,
  ): VulkanContextDescriptor =
    VulkanContextDescriptor(
      pointer,
      pointer,
      pointer,
      pointer,
      graphicsQueueFamilyIndex,
      pointer,
      pointer,
    )

  private fun vulkanBorrowedTextureDescriptor(
    pointer: NativePointer,
    format: Int = 0,
  ): VulkanBorrowedTextureDescriptor =
    VulkanBorrowedTextureDescriptor(
      RenderTargetExtent(1, 1, 1.0),
      1,
      1,
      vulkanContext(pointer),
      pointer,
      pointer,
      format,
      0,
    )
}
