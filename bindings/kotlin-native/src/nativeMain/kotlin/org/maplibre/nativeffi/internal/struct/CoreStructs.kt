package org.maplibre.nativeffi.internal.struct

import kotlinx.cinterop.CValue
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.MemScope
import kotlinx.cinterop.cValue
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.c.mln_edge_insets
import org.maplibre.nativeffi.internal.c.mln_lat_lng
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

  fun screenPoint(value: ScreenPoint): CValue<mln_screen_point> = cValue {
    x = value.x
    y = value.y
  }

  fun screenPoint(value: mln_screen_point): ScreenPoint = ScreenPoint(value.x, value.y)

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

  fun setStringView(native: mln_string_view, value: String, scope: MemScope) {
    native.data = MemoryUtil.utf8Bytes(scope, value)
    native.size = value.encodeToByteArray().size.toULong()
  }
}
