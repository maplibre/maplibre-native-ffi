package org.maplibre.nativeffi.map

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.browserTest
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.UnsupportedFeatureException
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.maplibreScope
import org.maplibre.nativeffi.render.MetalContextDescriptor
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.render.NativeBuffer
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.RenderBackend
import org.maplibre.nativeffi.render.RenderTargetExtent
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanContextDescriptor
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor
import org.maplibre.nativeffi.runtime.NetworkStatus
import org.maplibre.nativeffi.style.StyleLayerVisibility
import org.maplibre.nativeffi.withMap

/**
 * Inputs the binding refuses, and inputs it hands to native to refuse.
 *
 * The split is the whole point. A value the C ABI has no way to carry — a negative count for an
 * unsigned field, an enum sentinel from another revision — is refused here, before anything is
 * written into the module's heap. Everything else goes down and comes back as whatever native says,
 * because reimplementing native's validation is how a binding starts disagreeing with it.
 */
class DescriptorValidationBrowserTest {
  // Spec coverage: BND-068, BND-104, BND-160.

  @Test
  fun signedCarriersRefuseValuesTheirUnsignedFieldsCannotHold(): Promise<JsAny?> = browserTest {
    // Every one of these is a Kotlin Int or Long standing in for a C unsigned field. A negative
    // value reaches native as a very large positive one, so it is stopped here instead.
    assertFailsWith<InvalidArgumentException> {
      MapOptions().apply {
        width = -1
        height = 1
      }
    }
    assertFailsWith<InvalidArgumentException> { TileOptions().prefetchZoomDelta = -1 }
    assertFailsWith<InvalidArgumentException> { NativeBuffer.allocate(-1) }
    assertFailsWith<InvalidArgumentException> { RenderTargetExtent(-1, 1, 1.0) }
    assertFailsWith<InvalidArgumentException> { RenderTargetExtent(1, 1, 1.0).width = -1 }

    val nullPointer = NativePointer.NULL
    assertFailsWith<InvalidArgumentException> {
      vulkanContext(nullPointer, graphicsQueueFamilyIndex = -1)
    }
    assertFailsWith<InvalidArgumentException> {
      vulkanContext(nullPointer).graphicsQueueFamilyIndex = -1
    }
    assertFailsWith<InvalidArgumentException> { vulkanBorrowedTexture(nullPointer, format = -1) }
    assertFailsWith<InvalidArgumentException> { vulkanBorrowedTexture(nullPointer).format = -1 }
  }

  @Test
  fun anEnumSentinelTheCApiCannotBeGivenIsRefusedBeforeDispatch(): Promise<JsAny?> = browserTest {
    // An unknown value keeps its raw number, because it may be a real value from a later revision
    // read back out of native. What it may not do is go back down as input.
    assertEquals(900, MapMode(900).nativeValue)
    assertEquals(901, TileLodMode(901).nativeValue)
    assertEquals(902, NorthOrientation(902).nativeValue)
    assertEquals(903, ConstrainMode(903).nativeValue)
    assertEquals(904, ViewportMode(904).nativeValue)
    assertEquals(905, NetworkStatus(905).nativeValue)

    maplibreScope {
      withMap { runtime, map ->
        assertFailsWith<InvalidArgumentException> {
          MapHandle.create(runtime, MapOptions().apply { mapMode = MapMode(900) })
        }
        assertFailsWith<InvalidArgumentException> {
          map.tileOptions = TileOptions().apply { lodMode = TileLodMode(901) }
        }
        assertFailsWith<InvalidArgumentException> {
          map.viewportOptions = ViewportOptions().apply { northOrientation = NorthOrientation(902) }
        }
        assertFailsWith<InvalidArgumentException> {
          map.viewportOptions = ViewportOptions().apply { constrainMode = ConstrainMode(903) }
        }
        assertFailsWith<InvalidArgumentException> {
          map.viewportOptions = ViewportOptions().apply { viewportMode = ViewportMode(904) }
        }
        assertFailsWith<InvalidArgumentException> { Maplibre.setNetworkStatus(NetworkStatus(905)) }

        // The map is unchanged by any of it, so each refusal happened before the call.
        map.tileOptions = TileOptions()
        map.viewportOptions = ViewportOptions()
      }
    }
  }

  @Test
  fun invalidMapAndValueInputsCarryNativesOwnRefusal(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap { _, map ->
        map.setStyleJson(EMPTY_STYLE_JSON)

        // Coordinate validation is native's, and reaches the caller as the public error shape for
        // the status native returned, carrying native's own words for it.
        val coordinate =
          assertFailsWith<InvalidArgumentException> { map.pixelForLatLng(LatLng(Double.NaN, 0.0)) }
        assertEquals(MaplibreStatus.INVALID_ARGUMENT, coordinate.status)
        assertTrue(coordinate.diagnostic.contains("latitude"), coordinate.diagnostic)

        val projection = map.createProjection()
        try {
          val projected =
            assertFailsWith<InvalidArgumentException> {
              projection.pixelForLatLng(LatLng(Double.NaN, 0.0))
            }
          assertTrue(projected.diagnostic.contains("latitude"), projected.diagnostic)
        } finally {
          projection.close()
        }

        // This one is a process-global entry point, so it runs on the page rather than on the
        // owner thread. Its message comes from the page's own diagnostic slot, and it says the
        // same thing.
        val meters =
          assertFailsWith<InvalidArgumentException> {
            Maplibre.projectedMetersForLatLng(LatLng(Double.NaN, 0.0))
          }
        assertTrue(meters.diagnostic.contains("latitude"), meters.diagnostic)

        // A structured value native refuses: JSON has no non-finite number, and the binding does
        // not pre-empt that check.
        val nonFinite =
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
        // Only that a message arrived: the wording of a style-value refusal is MapLibre's and
        // moves with it, while what this covers is that native's message reaches the caller.
        assertTrue(nonFinite.diagnostic.isNotEmpty(), "diagnostic was empty")

        // An unknown enum that the binding does not own an invariant for goes down and is refused
        // there instead.
        map.addStyleLayerJson(
          JsonValue.ObjectValue(
            listOf(
              JsonValue.Member("id", JsonValue.StringValue("bg")),
              JsonValue.Member("type", JsonValue.StringValue("background")),
            )
          ),
          "",
        )
        assertFailsWith<InvalidArgumentException> {
          map.setLayerVisibility("bg", StyleLayerVisibility(900))
        }
      }
    }
  }

  @Test
  fun aBackendThisModuleWasNotBuiltWithIsRefusedBeforeASessionExists(): Promise<JsAny?> =
    browserTest {
      maplibreScope {
        withMap { _, map ->
          val supported = Maplibre.supportedRenderBackends()
          assertEquals(setOf(RenderBackend.OPENGL), supported)

          val extent = RenderTargetExtent(64, 64, 1.0)
          val pointer = NativePointer.ofAddress(0x10L)

          val metal =
            assertFailsWith<UnsupportedFeatureException> {
              map.attachMetalOwnedTexture(
                MetalOwnedTextureDescriptor(extent, MetalContextDescriptor(pointer))
              )
            }
          assertEquals(MaplibreStatus.UNSUPPORTED, metal.status)

          val vulkan =
            assertFailsWith<UnsupportedFeatureException> {
              map.attachVulkanOwnedTexture(
                VulkanOwnedTextureDescriptor(extent, context = vulkanContext(pointer))
              )
            }
          assertEquals(MaplibreStatus.UNSUPPORTED, vulkan.status)

          // Refused before a session existed, so the map is still free to take one.
          assertEquals(false, map.isClosed)
        }
      }
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

  private fun vulkanBorrowedTexture(
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
