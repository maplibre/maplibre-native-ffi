package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertFailsWith
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.memScoped
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.internal.struct.MapStructs
import org.maplibre.nativeffi.internal.struct.ResourceStructs
import org.maplibre.nativeffi.internal.struct.StyleStructs
import org.maplibre.nativeffi.resource.ResourceErrorReason
import org.maplibre.nativeffi.resource.ResourceResponse
import org.maplibre.nativeffi.resource.ResourceResponseStatus

@OptIn(ExperimentalForeignApi::class)
class NativeDescriptorValidationTest : org.maplibre.nativeffi.NativeTestBase() {
  @Test
  fun enumInputsRejectUnknownSentinelsBeforeNativeCalls() {
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
}
