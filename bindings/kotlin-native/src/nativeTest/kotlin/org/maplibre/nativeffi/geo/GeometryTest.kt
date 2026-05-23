package org.maplibre.nativeffi.geo

import kotlin.test.Test
import kotlin.test.assertEquals

class GeometryTest {
  @Test
  fun geometryFactoriesCopyNestedCollections() {
    val ring = mutableListOf(LatLng(0.0, 0.0), LatLng(1.0, 1.0))
    val rings = mutableListOf(ring)
    val polygon = Geometry.polygon(rings)

    ring.add(LatLng(2.0, 2.0))
    rings.add(mutableListOf(LatLng(3.0, 3.0)))

    assertEquals(1, polygon.rings.size)
    assertEquals(2, polygon.rings.single().size)
  }
}
