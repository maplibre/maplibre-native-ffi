#define MLN_BUILDING_C

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "bytes/buffer.hpp"
#include "c_api/boundary.hpp"
#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#include "maplibre_native_c.h"
#include "operation/operation.hpp"
#include "runtime/runtime.hpp"

auto mln_map_projection_create_start(
  mln_map map, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_projection_create_start(map, out_operation);
  });
}

auto mln_map_projection_create_take_result(
  mln_operation operation, mln_map_projection* out_projection
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_projection_create_take_result(
      operation, out_projection
    );
  });
}

auto mln_map_projection_close_start(
  mln_map_projection projection, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_projection_close_start(projection, out_operation);
  });
}

auto mln_map_projection_get_camera_start(
  mln_map_projection projection, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_projection_get_camera_start(
      projection, out_operation
    );
  });
}

auto mln_map_projection_get_camera_take_result(
  mln_operation operation, mln_camera_options* out_camera
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_projection_get_camera_take_result(
      operation, out_camera
    );
  });
}

auto mln_map_projection_set_camera(
  mln_map_projection projection, const mln_camera_options* camera,
  uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_projection_set_camera(
      projection, camera, out_command_id
    );
  });
}

auto mln_map_projection_set_visible_coordinates(
  mln_map_projection projection, const mln_lat_lng* coordinates,
  size_t coordinate_count, mln_edge_insets padding, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_projection_set_visible_coordinates(
      projection, coordinates, coordinate_count, padding, out_command_id
    );
  });
}

auto mln_map_projection_set_visible_geometry(
  mln_map_projection projection, mln_buffer_view geometry,
  mln_edge_insets padding, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_projection_set_visible_geometry(
      projection, geometry, padding, out_command_id
    );
  });
}

auto mln_map_projection_pixel_for_lat_lng_start(
  mln_map_projection projection, mln_lat_lng coordinate,
  mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_projection_pixel_for_lat_lng_start(
      projection, coordinate, out_operation
    );
  });
}

auto mln_map_projection_pixel_for_lat_lng_take_result(
  mln_operation operation, mln_screen_point* out_point
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_projection_pixel_for_lat_lng_take_result(
      operation, out_point
    );
  });
}

auto mln_map_projection_lat_lng_for_pixel_start(
  mln_map_projection projection, mln_screen_point point,
  mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_projection_lat_lng_for_pixel_start(
      projection, point, out_operation
    );
  });
}

auto mln_map_projection_lat_lng_for_pixel_take_result(
  mln_operation operation, mln_lat_lng* out_coordinate
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_projection_lat_lng_for_pixel_take_result(
      operation, out_coordinate
    );
  });
}

auto mln_projected_meters_for_lat_lng(
  mln_lat_lng coordinate, mln_projected_meters* out_meters
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::projected_meters_for_lat_lng(coordinate, out_meters);
  });
}

auto mln_lat_lng_for_projected_meters(
  mln_projected_meters meters, mln_lat_lng* out_coordinate
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::lat_lng_for_projected_meters(meters, out_coordinate);
  });
}