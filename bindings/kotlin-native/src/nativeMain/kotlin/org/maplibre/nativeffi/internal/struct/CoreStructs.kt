package org.maplibre.nativeffi.internal.struct

import kotlinx.cinterop.CPointer
import kotlinx.cinterop.CValue
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.MemScope
import kotlinx.cinterop.allocArray
import kotlinx.cinterop.cValue
import kotlinx.cinterop.get
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.geo.ProjectedMeters
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.c.mln_edge_insets
import org.maplibre.nativeffi.internal.c.mln_lat_lng
import org.maplibre.nativeffi.internal.c.mln_lat_lng_bounds
import org.maplibre.nativeffi.internal.c.mln_projected_meters
import org.maplibre.nativeffi.internal.c.mln_screen_point
import org.maplibre.nativeffi.internal.c.mln_string_view
import org.maplibre.nativeffi.internal.memory.MemoryUtil

/** Materializes core copied values at the C boundary. */
@OptIn(ExperimentalForeignApi::class)
internal object CoreStructs {
  fun latLng(value: LatLng): CValue<mln_lat_lng> = cValue {
    latitude = value.latitude
    longitude = value.longitude
  }

  fun latLng(value: mln_lat_lng): LatLng = LatLng(value.latitude, value.longitude)

  fun latLngArray(values: List<LatLng>, scope: MemScope): CPointer<mln_lat_lng>? {
    if (values.isEmpty()) return null
    val array = scope.allocArray<mln_lat_lng>(values.size)
    values.forEachIndexed { index, value ->
      array[index].latitude = value.latitude
      array[index].longitude = value.longitude
    }
    return array
  }

  fun latLngArray(values: CPointer<mln_lat_lng>?, count: Int): List<LatLng> =
    if (values == null || count == 0) emptyList()
    else List(count) { index -> latLng(values[index]) }

  fun latLngBounds(value: LatLngBounds): CValue<mln_lat_lng_bounds> = cValue {
    southwest.latitude = value.southwest.latitude
    southwest.longitude = value.southwest.longitude
    northeast.latitude = value.northeast.latitude
    northeast.longitude = value.northeast.longitude
  }

  fun latLngBounds(value: mln_lat_lng_bounds): LatLngBounds =
    LatLngBounds(latLng(value.southwest), latLng(value.northeast))

  fun projectedMeters(value: ProjectedMeters): CValue<mln_projected_meters> = cValue {
    northing = value.northing
    easting = value.easting
  }

  fun projectedMeters(value: mln_projected_meters): ProjectedMeters =
    ProjectedMeters(value.northing, value.easting)

  fun screenPoint(value: ScreenPoint): CValue<mln_screen_point> = cValue {
    x = value.x
    y = value.y
  }

  fun screenPoint(value: mln_screen_point): ScreenPoint = ScreenPoint(value.x, value.y)

  fun screenPointArray(values: List<ScreenPoint>, scope: MemScope): CPointer<mln_screen_point>? {
    if (values.isEmpty()) return null
    val array = scope.allocArray<mln_screen_point>(values.size)
    values.forEachIndexed { index, value ->
      array[index].x = value.x
      array[index].y = value.y
    }
    return array
  }

  fun screenPointArray(values: CPointer<mln_screen_point>?, count: Int): List<ScreenPoint> =
    if (values == null || count == 0) emptyList()
    else List(count) { index -> screenPoint(values[index]) }

  fun edgeInsets(value: EdgeInsets): CValue<mln_edge_insets> = cValue {
    top = value.top
    left = value.left
    bottom = value.bottom
    right = value.right
  }

  fun edgeInsets(value: mln_edge_insets): EdgeInsets =
    EdgeInsets(value.top, value.left, value.bottom, value.right)

  fun stringView(value: String, scope: MemScope): CValue<mln_string_view> = cValue {
    data = MemoryUtil.utf8Bytes(scope, value)
    size = value.encodeToByteArray().size.toULong()
  }

  fun stringView(value: mln_string_view): String = MemoryUtil.copyStringView(value.data, value.size)

  fun setStringView(native: mln_string_view, value: String, scope: MemScope) {
    native.data = MemoryUtil.utf8Bytes(scope, value)
    native.size = value.encodeToByteArray().size.toULong()
  }
}
