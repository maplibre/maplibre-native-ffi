package org.maplibre.nativeffi.internal.struct

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNotNull
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.internal.c.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY
import org.maplibre.nativeffi.internal.c.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID
import org.maplibre.nativeffi.offline.OfflineRegionDefinition

@OptIn(ExperimentalForeignApi::class)
class RuntimeOfflineStructsTest {
  @Test
  fun offlineRegionDefinitionMaterializesTilePyramidAndGeometryVariants() {
    memScoped {
      val tilePyramid =
        RuntimeStructs.offlineRegionDefinition(
            OfflineRegionDefinition.TilePyramid(
              "asset://style.json",
              LatLngBounds(LatLng(1.0, 2.0), LatLng(3.0, 4.0)),
              1.0,
              5.0,
              2.0f,
              true,
            ),
            this,
          )
          .pointed
      assertEquals(MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID, tilePyramid.type)
      assertEquals(1.0, tilePyramid.data.tile_pyramid.bounds.southwest.latitude)
      assertEquals(4.0, tilePyramid.data.tile_pyramid.bounds.northeast.longitude)
      assertEquals(2.0f, tilePyramid.data.tile_pyramid.pixel_ratio)

      val geometry =
        RuntimeStructs.offlineRegionDefinition(
            OfflineRegionDefinition.GeometryRegion(
              "asset://style.json",
              Geometry.point(LatLng(5.0, 6.0)),
              2.0,
              6.0,
              1.0f,
              false,
            ),
            this,
          )
          .pointed
      assertEquals(MLN_OFFLINE_REGION_DEFINITION_GEOMETRY, geometry.type)
      assertNotNull(geometry.data.geometry.geometry)
    }
  }

  @Test
  fun offlineMetadataUsesNullPointerOnlyForEmptyMetadata() {
    memScoped {
      assertEquals(null, RuntimeStructs.metadata(ByteArray(0), this))
      assertNotNull(RuntimeStructs.metadata(byteArrayOf(1, 2, 3), this))
    }
  }
}
