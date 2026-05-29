package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertFailsWith
import kotlinx.cinterop.ExperimentalForeignApi
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.internal.struct.StyleStructs
import org.maplibre.nativeffi.render.NativeBuffer
import org.maplibre.nativeffi.render.RenderTargetExtent
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanContextDescriptor
import org.maplibre.nativeffi.resource.ResourceErrorReason
import org.maplibre.nativeffi.resource.ResourceResponse
import org.maplibre.nativeffi.runtime.RuntimeOptions

@OptIn(ExperimentalForeignApi::class)
class DescriptorValidationTest {
  @Test
  fun signedConvenienceSettersRejectNegativeValues() {
    assertFailsWith<IllegalArgumentException> {
      MapOptions().apply {
        width = -1
        height = 1
      }
    }
    assertFailsWith<IllegalArgumentException> { TileOptions().prefetchZoomDelta = -1 }
    assertFailsWith<IllegalArgumentException> { RuntimeOptions().maximumCacheSize = -1L }
    assertFailsWith<IllegalArgumentException> { NativeBuffer.allocate(-1) }
    assertFailsWith<IllegalArgumentException> { RenderTargetExtent(-1, 1) }
    assertFailsWith<IllegalArgumentException> { RenderTargetExtent().width = -1 }
    assertFailsWith<IllegalArgumentException> {
      VulkanContextDescriptor(graphicsQueueFamilyIndex = -1)
    }
    assertFailsWith<IllegalArgumentException> {
      VulkanContextDescriptor().graphicsQueueFamilyIndex = -1
    }
    assertFailsWith<IllegalArgumentException> { VulkanBorrowedTextureDescriptor(format = -1) }
    assertFailsWith<IllegalArgumentException> { VulkanBorrowedTextureDescriptor().format = -1 }
  }

  @Test
  fun enumInputsKeepUnknownSentinelsUntilNativeValidation() {
    MapOptions().mapMode = MapMode.UNKNOWN
    TileOptions().lodMode = TileLodMode.UNKNOWN
    ViewportOptions().northOrientation = NorthOrientation.UNKNOWN
    ViewportOptions().constrainMode = ConstrainMode.UNKNOWN
    ViewportOptions().viewportMode = ViewportMode.UNKNOWN
    ResourceResponse.error(ResourceErrorReason.UNKNOWN, "bad")
  }

  @Test
  fun canonicalTileMaterializationRejectsOverflow() {
    assertFailsWith<IllegalArgumentException> {
      StyleStructs.canonicalTileId(CanonicalTileId(0, UInt.MAX_VALUE.toLong() + 1, 0))
    }
  }
}
