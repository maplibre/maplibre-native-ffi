package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.memScoped
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.UnsupportedFeatureException
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.internal.struct.MapStructs
import org.maplibre.nativeffi.internal.struct.ResourceStructs
import org.maplibre.nativeffi.internal.struct.StyleStructs
import org.maplibre.nativeffi.json.JsonValue
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
import org.maplibre.nativeffi.resource.ResourceErrorReason
import org.maplibre.nativeffi.resource.ResourceResponse
import org.maplibre.nativeffi.resource.ResourceResponseStatus
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

@OptIn(ExperimentalForeignApi::class)
class DescriptorValidationTest : org.maplibre.nativeffi.NativeTestBase() {
  // BND-104: invalid map, projection, descriptor, and structured-value inputs report public errors.

  @Test
  fun signedCarriersRejectNegativeUnsignedValues() {
    assertFailsWith<InvalidArgumentException> {
      MapOptions().apply {
        width = -1
        height = 1
      }
    }
    assertFailsWith<InvalidArgumentException> { TileOptions().prefetchZoomDelta = -1 }
    assertFailsWith<InvalidArgumentException> { RuntimeOptions().maximumCacheSize = -1L }
    assertFailsWith<IllegalArgumentException> { NativeBuffer.allocate(-1) }
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
  fun enumInputsRejectUnknownSentinelsBeforeNativeCalls() {
    // BND-068: unknown enum values are rejected before they cross into C input APIs.
    assertFailsWith<InvalidArgumentException> {
      MapHandle.mapOptionsForTesting(MapOptions().apply { mapMode = MapMode(900) }) {}
    }
    memScoped {
      assertFailsWith<InvalidArgumentException> {
        MapStructs.tileOptions(TileOptions().apply { lodMode = TileLodMode(901) }, this)
      }
      assertFailsWith<InvalidArgumentException> {
        MapStructs.viewportOptions(
          ViewportOptions().apply { northOrientation = NorthOrientation(902) },
          this,
        )
      }
      assertFailsWith<InvalidArgumentException> {
        MapStructs.viewportOptions(
          ViewportOptions().apply { constrainMode = ConstrainMode(903) },
          this,
        )
      }
      assertFailsWith<InvalidArgumentException> {
        MapStructs.viewportOptions(
          ViewportOptions().apply { viewportMode = ViewportMode(904) },
          this,
        )
      }
      assertFailsWith<InvalidArgumentException> {
        ResourceStructs.resourceResponse(
          ResourceResponse(ResourceResponseStatus.ERROR).apply {
            errorReason = ResourceErrorReason(905)
            errorMessage = "bad"
          },
          this,
        )
      }
    }
  }

  @Test
  fun canonicalTileMaterializationRejectsOverflow() {
    assertFailsWith<InvalidArgumentException> {
      StyleStructs.canonicalTileId(CanonicalTileId(0, UInt.MAX_VALUE.toLong() + 1, 0))
    }
  }

  @Test
  fun structuredJsonInputsPropagateNativeFiniteNumberValidation() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 128
          height = 128
        },
      )
    try {
      map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}")

      val error =
        assertFailsWith<InvalidArgumentException> {
          map.addStyleLayerJson(
            JsonValue.ObjectValue(
              listOf(
                JsonValue.Member("id", JsonValue.StringValue("invalid-background")),
                JsonValue.Member("type", JsonValue.StringValue("background")),
                JsonValue.Member(
                  "paint",
                  JsonValue.ObjectValue(
                    listOf(
                      JsonValue.Member("background-opacity", JsonValue.DoubleValue(Double.NaN))
                    )
                  ),
                ),
              )
            ),
            "",
          )
        }

      assertEquals(MaplibreStatus.INVALID_ARGUMENT, error.status)
      assertTrue(error.diagnostic.contains("double value must be finite"))
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun mapAndProjectionInputsPropagateNativeCoordinateValidation() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 128
          height = 128
        },
      )
    var projection: MapProjectionHandle? = null
    try {
      val invalidCoordinate = LatLng(Double.NaN, 0.0)
      assertInvalidCoordinateDiagnostic { map.pixelForLatLng(invalidCoordinate) }

      projection = map.createProjection()
      assertInvalidCoordinateDiagnostic { projection.pixelForLatLng(invalidCoordinate) }
      assertInvalidCoordinateDiagnostic { Maplibre.projectedMetersForLatLng(invalidCoordinate) }
    } finally {
      projection?.close()
      map.close()
      runtime.close()
    }
  }

  @Test
  fun unsupportedRenderBackendsRejectAttachBeforeSessionCreation() {
    val supported = Maplibre.supportedRenderBackends()
    assertTrue(supported.isNotEmpty())

    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 128
          height = 128
        },
      )
    try {
      val fakePointer = NativePointer.ofAddress(0x10L)
      val fakeExtent = RenderTargetExtent(256, 256, 1.0)
      if (RenderBackend.METAL !in supported) {
        val error =
          assertFailsWith<UnsupportedFeatureException> {
            map.attachMetalOwnedTexture(
              MetalOwnedTextureDescriptor(fakeExtent, MetalContextDescriptor(fakePointer))
            )
          }
        assertEquals(MaplibreStatus.UNSUPPORTED, error.status)
      }
      if (RenderBackend.VULKAN !in supported) {
        val error =
          assertFailsWith<UnsupportedFeatureException> {
            map.attachVulkanOwnedTexture(
              VulkanOwnedTextureDescriptor(fakeExtent, context = vulkanContext(fakePointer))
            )
          }
        assertEquals(MaplibreStatus.UNSUPPORTED, error.status)
      }
      if (RenderBackend.OPENGL !in supported) {
        val error =
          assertFailsWith<UnsupportedFeatureException> {
            map.attachOpenGLOwnedTexture(
              OpenGLOwnedTextureDescriptor(
                fakeExtent,
                context = WglContextDescriptor(fakePointer, fakePointer, fakePointer),
              )
            )
          }
        assertEquals(MaplibreStatus.UNSUPPORTED, error.status)
      }
    } finally {
      map.close()
      runtime.close()
    }
  }

  private fun assertInvalidCoordinateDiagnostic(block: () -> Unit) {
    val error = assertFailsWith<InvalidArgumentException> { block() }
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
      vulkanContext(pointer),
      pointer,
      pointer,
      format,
      0,
    )
}
