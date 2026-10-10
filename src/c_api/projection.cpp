#define MLN_BUILDING_C

#include <cstddef>

#include "c_api/boundary.hpp"
#include "map/map.hpp"
#include "maplibre_native_c.h"

auto mln_map_projection_create(
  mln_map map, mln_map_projection* out_projection,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::map_projection_create(map, out_projection);
  });
}

auto mln_map_projection_close(
  mln_map_projection projection, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::map_projection_close(projection);
  });
}

auto mln_map_projection_get_camera(
  mln_map_projection projection, mln_camera_options* out_camera,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::map_projection_get_camera(projection, out_camera);
  });
}

auto mln_map_projection_set_camera(
  mln_map_projection projection, const mln_camera_options* camera,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::map_projection_set_camera(projection, camera);
  });
}

auto mln_map_projection_set_visible_coordinates(
  mln_map_projection projection, const mln_lat_lng* coordinates,
  size_t coordinate_count, mln_edge_insets padding,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::map_projection_set_visible_coordinates(
      projection, coordinates, coordinate_count, padding
    );
  });
}

auto mln_map_projection_set_visible_geometry(
  mln_map_projection projection, mln_buffer_view geometry,
  mln_edge_insets padding, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::map_projection_set_visible_geometry(
      projection, geometry, padding
    );
  });
}

auto mln_map_projection_pixel_for_lat_lng(
  mln_map_projection projection, mln_lat_lng coordinate,
  mln_screen_point* out_point, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::map_projection_pixel_for_lat_lng(
      projection, coordinate, out_point
    );
  });
}

auto mln_map_projection_lat_lng_for_pixel(
  mln_map_projection projection, mln_screen_point point,
  mln_lat_lng* out_coordinate, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::map_projection_lat_lng_for_pixel(
      projection, point, out_coordinate
    );
  });
}

auto mln_map_projection_lat_lng_for_pixel_unwrapped(
  mln_map_projection projection, mln_screen_point point,
  mln_lat_lng* out_coordinate, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::map_projection_lat_lng_for_pixel_unwrapped(
      projection, point, out_coordinate
    );
  });
}

auto mln_projected_meters_for_lat_lng(
  mln_lat_lng coordinate, mln_projected_meters* out_meters,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::projected_meters_for_lat_lng(coordinate, out_meters);
  });
}

auto mln_lat_lng_for_projected_meters(
  mln_projected_meters meters, mln_lat_lng* out_coordinate,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::lat_lng_for_projected_meters(meters, out_coordinate);
  });
}

auto mln_map_projection_meters_per_pixel_at_latitude(
  mln_map_projection projection, double latitude, double* out_meters_per_pixel,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::map_projection_meters_per_pixel_at_latitude(
      projection, latitude, out_meters_per_pixel
    );
  });
}
