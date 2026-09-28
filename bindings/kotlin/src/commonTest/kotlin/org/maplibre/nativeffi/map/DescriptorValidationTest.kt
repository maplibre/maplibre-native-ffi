package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.UnsupportedFeatureException
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.LatLng
import org.maplibre.nativeffi.generated.MapOptions
import org.maplibre.nativeffi.generated.MapProjectionHandle
import org.maplibre.nativeffi.generated.MetalContextDescriptor
import org.maplibre.nativeffi.generated.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.OpenglContextDescriptor
import org.maplibre.nativeffi.generated.OpenglContextDescriptorData
import org.maplibre.nativeffi.generated.OpenglContextOwnership
import org.maplibre.nativeffi.generated.OpenglOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.RenderBackendFlag
import org.maplibre.nativeffi.generated.RenderDriverKind
import org.maplibre.nativeffi.generated.RenderSessionAttachOptions
import org.maplibre.nativeffi.generated.RenderTargetExtent
import org.maplibre.nativeffi.generated.VulkanContextDescriptor
import org.maplibre.nativeffi.generated.VulkanOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.WglContextDescriptor
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.runtime.runSuspendTest

class DescriptorValidationTest {
  @Test
  fun mapAndProjectionInputsPropagateNativeCoordinateValidation(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map = runtime.mapCreate(mapOptions()).await()
    var projection: MapProjectionHandle? = null
    try {
      val invalidCoordinate = LatLng(Double.NaN, 0.0)
      val createdProjection = map.projectionCreate().await()
      projection = createdProjection
      assertInvalidCoordinateDiagnostic { createdProjection.pixelForLatLng(invalidCoordinate) }
      assertInvalidCoordinateDiagnostic { GeneratedApi.projectedMetersForLatLng(invalidCoordinate) }
    } finally {
      projection?.close()
      map.release().await()
      runtime.release().await()
    }
  }

  @Test
  fun unsupportedRenderBackendFlagsRejectAttachBeforeSessionCreation(): Unit = runSuspendTest {
    val supported = GeneratedApi.supportedRenderBackendMask()
    assertTrue(supported.rawValue != 0u)
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map = runtime.mapCreate(mapOptions()).await()
    try {
      val pointer = NativePointer.ofAddress(0x10L)
      val extent = RenderTargetExtent(256u, 256u, 1.0)
      if (RenderBackendFlag.METAL !in supported) {
        assertEquals(
          MaplibreStatus.UNSUPPORTED,
          assertFailsWith<UnsupportedFeatureException> {
              map.metalOwnedTextureAttach(
                MetalOwnedTextureDescriptor(extent, MetalContextDescriptor(pointer)),
                RenderSessionAttachOptions(RenderDriverKind.CALLER_GRAPHICS_THREAD),
              )
            }
            .status,
        )
      }
      if (RenderBackendFlag.VULKAN !in supported) {
        assertEquals(
          MaplibreStatus.UNSUPPORTED,
          assertFailsWith<UnsupportedFeatureException> {
              map.vulkanOwnedTextureAttach(
                VulkanOwnedTextureDescriptor(extent, context = vulkanContext(pointer)),
                RenderSessionAttachOptions(RenderDriverKind.CALLER_GRAPHICS_THREAD),
              )
            }
            .status,
        )
      }
      if (RenderBackendFlag.OPENGL !in supported) {
        assertEquals(
          MaplibreStatus.UNSUPPORTED,
          assertFailsWith<UnsupportedFeatureException> {
              map.openglOwnedTextureAttach(
                OpenglOwnedTextureDescriptor(
                  extent,
                  context =
                    OpenglContextDescriptor(
                      OpenglContextOwnership.SHARED,
                      OpenglContextDescriptorData.Wgl(
                        WglContextDescriptor(pointer, pointer, pointer)
                      ),
                    ),
                ),
                RenderSessionAttachOptions(RenderDriverKind.CALLER_GRAPHICS_THREAD),
              )
            }
            .status,
        )
      }
    } finally {
      map.release().await()
      runtime.release().await()
    }
  }

  private fun mapOptions(): MapOptions =
    GeneratedApi.mapOptionsDefault()
      .copy(
        initialExtent =
          GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 128u, height = 128u)
      )

  private suspend fun assertInvalidCoordinateDiagnostic(block: suspend () -> Unit) {
    val error = assertFailsWith<InvalidArgumentException> { block() }
    assertEquals(MaplibreStatus.INVALID_ARGUMENT, error.status)
    assertTrue(error.diagnostic.contains("latitude must be finite"))
  }

  private fun vulkanContext(
    pointer: NativePointer,
    graphicsQueueFamilyIndex: UInt = 0u,
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
}
