package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.geo.GeoJson
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.style.SourceType

class StyleHandleTest {
  @Test
  fun styleSourceAndLayerJsonApisCallNativeAndCopyDescriptors() {
    val runtime = RuntimeHandle.create()
    val map = MapHandle.create(runtime, MapOptions().size(128, 128))
    try {
      map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}")
      map.addStyleSourceJson(
        "parks",
        JsonValue.obj(
          listOf(
            JsonValue.Member("type", JsonValue.of("geojson")),
            JsonValue.Member(
              "data",
              JsonValue.obj(
                listOf(
                  JsonValue.Member("type", JsonValue.of("FeatureCollection")),
                  JsonValue.Member("features", JsonValue.array(emptyList())),
                )
              ),
            ),
          )
        ),
      )
      assertTrue(map.styleSourceExists("parks"))
      assertEquals(SourceType.GEOJSON, map.styleSourceType("parks"))
      assertEquals(SourceType.GEOJSON, map.styleSourceInfo("parks")?.type)
      assertTrue(map.styleSourceIds().contains("parks"))

      map.addStyleLayerJson(
        JsonValue.obj(
          listOf(
            JsonValue.Member("id", JsonValue.of("park-circles")),
            JsonValue.Member("type", JsonValue.of("circle")),
            JsonValue.Member("source", JsonValue.of("parks")),
          )
        )
      )
      assertTrue(map.styleLayerExists("park-circles"))
      map.setLayerProperty("park-circles", "circle-radius", JsonValue.of(5.0))
      map.setLayerFilter(
        "park-circles",
        JsonValue.array(listOf(JsonValue.of("has"), JsonValue.of("kind"))),
      )
      assertTrue(map.removeStyleLayer("park-circles"))
      assertFalse(map.styleLayerExists("park-circles"))
      assertTrue(map.removeStyleSource("parks"))
      assertFalse(map.styleSourceExists("parks"))
    } finally {
      map.close()
      runtime.close()
    }
  }

  @Test
  fun geoJsonSourceApisMaterializeGeoJsonDescriptors() {
    val runtime = RuntimeHandle.create()
    val map = MapHandle.create(runtime, MapOptions().size(128, 128))
    try {
      map.setStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}")
      map.addGeoJsonSourceData(
        "points",
        GeoJson.featureCollection(
          listOf(
            Feature(
              Geometry.point(LatLng(0.0, 0.0)),
              listOf(JsonValue.Member("kind", JsonValue.of("point"))),
            )
          )
        ),
      )
      assertEquals(SourceType.GEOJSON, map.styleSourceType("points"))
      map.setGeoJsonSourceData("points", GeoJson.geometry(Geometry.point(LatLng(1.0, 1.0))))
      assertTrue(map.removeStyleSource("points"))
    } finally {
      map.close()
      runtime.close()
    }
  }
}
