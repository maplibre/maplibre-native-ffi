#pragma once

#include <maplibre_native_c/map.h>
#include <maplibre_native_c/projection.h>

// The private Android bridge keeps temporary structs on the native stack.
// The caller supplies 15 doubles: fields, center, altitude, padding, anchor,
// zoom, bearing, pitch, roll, and field of view, in that order.
inline void mln_android_copy_camera(
  const mln_camera_options& camera, double* out
) {
  out[0] = camera.fields;
  out[1] = camera.latitude;
  out[2] = camera.longitude;
  out[3] = camera.center_altitude;
  out[4] = camera.padding.top;
  out[5] = camera.padding.left;
  out[6] = camera.padding.bottom;
  out[7] = camera.padding.right;
  out[8] = camera.anchor.x;
  out[9] = camera.anchor.y;
  out[10] = camera.zoom;
  out[11] = camera.bearing;
  out[12] = camera.pitch;
  out[13] = camera.roll;
  out[14] = camera.field_of_view;
}

inline mln_status mln_android_map_get_camera(mln_map map, double* out) {
  auto camera = mln_camera_options_default();
  const auto status = mln_map_get_camera(map, &camera);
  if (status == MLN_STATUS_OK) mln_android_copy_camera(camera, out);
  return status;
}

inline mln_status mln_android_projection_get_camera(
  mln_map_projection projection, double* out
) {
  auto camera = mln_camera_options_default();
  const auto status = mln_map_projection_get_camera(projection, &camera);
  if (status == MLN_STATUS_OK) mln_android_copy_camera(camera, out);
  return status;
}

inline mln_status mln_android_map_pixel_for_lat_lng(
  mln_map map, double latitude, double longitude, double* out
) {
  mln_screen_point point{};
  const auto status =
    mln_map_pixel_for_lat_lng(map, {latitude, longitude}, &point);
  if (status == MLN_STATUS_OK) {
    out[0] = point.x;
    out[1] = point.y;
  }
  return status;
}

inline mln_status mln_android_projection_pixel_for_lat_lng(
  mln_map_projection projection, double latitude, double longitude, double* out
) {
  mln_screen_point point{};
  const auto status = mln_map_projection_pixel_for_lat_lng(
    projection, {latitude, longitude}, &point
  );
  if (status == MLN_STATUS_OK) {
    out[0] = point.x;
    out[1] = point.y;
  }
  return status;
}

inline mln_status mln_android_map_lat_lng_for_pixel(
  mln_map map, double x, double y, bool unwrapped, double* out
) {
  mln_lat_lng coordinate{};
  const auto status =
    unwrapped ? mln_map_lat_lng_for_pixel_unwrapped(map, {x, y}, &coordinate)
              : mln_map_lat_lng_for_pixel(map, {x, y}, &coordinate);
  if (status == MLN_STATUS_OK) {
    out[0] = coordinate.latitude;
    out[1] = coordinate.longitude;
  }
  return status;
}

inline mln_status mln_android_projection_lat_lng_for_pixel(
  mln_map_projection projection, double x, double y, bool unwrapped, double* out
) {
  mln_lat_lng coordinate{};
  const auto status =
    unwrapped
      ? mln_map_projection_lat_lng_for_pixel_unwrapped(
          projection, {x, y}, &coordinate
        )
      : mln_map_projection_lat_lng_for_pixel(projection, {x, y}, &coordinate);
  if (status == MLN_STATUS_OK) {
    out[0] = coordinate.latitude;
    out[1] = coordinate.longitude;
  }
  return status;
}
