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

auto mln_camera_options_default(void) noexcept -> mln_camera_options {
  return mln::core::camera_options_default();
}

auto mln_animation_options_default(void) noexcept -> mln_animation_options {
  return mln::core::animation_options_default();
}
auto mln_camera_update_default(void) noexcept -> mln_camera_update {
  return mln::core::camera_update_default();
}

auto mln_camera_fit_options_default(void) noexcept -> mln_camera_fit_options {
  return mln::core::camera_fit_options_default();
}

auto mln_bound_options_default(void) noexcept -> mln_bound_options {
  return mln::core::bound_options_default();
}

auto mln_free_camera_options_default(void) noexcept -> mln_free_camera_options {
  return mln::core::free_camera_options_default();
}

auto mln_projection_mode_default(void) noexcept -> mln_projection_mode {
  return mln::core::projection_mode_default();
}

auto mln_map_viewport_options_default(void) noexcept
  -> mln_map_viewport_options {
  return mln::core::map_viewport_options_default();
}

auto mln_map_tile_options_default(void) noexcept -> mln_map_tile_options {
  return mln::core::map_tile_options_default();
}

auto mln_map_camera_snapshot_get(
  mln_map map, mln_camera_options* out_camera, uint64_t* out_generation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_camera_snapshot_get(map, out_camera, out_generation);
  });
}

auto mln_map_update_camera(
  mln_map map, const mln_camera_update* update, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_update_camera(map, update, out_command_id);
  });
}

auto mln_map_move_by(
  mln_map map, mln_screen_point offset, const mln_animation_options* animation,
  uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_move_by(map, offset, animation, out_command_id);
  });
}

auto mln_map_scale_by(
  mln_map map, double scale, const mln_screen_point* anchor,
  const mln_animation_options* animation, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_scale_by(
      map, scale, anchor, animation, out_command_id
    );
  });
}

auto mln_map_bearing_by(
  mln_map map, double degrees, const mln_screen_point* anchor,
  const mln_animation_options* animation, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_bearing_by(
      map, degrees, anchor, animation, out_command_id
    );
  });
}

auto mln_map_pitch_by(
  mln_map map, double degrees, const mln_animation_options* animation,
  uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_pitch_by(map, degrees, animation, out_command_id);
  });
}

auto mln_map_camera_query_start(
  mln_map map, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_camera_query_start(map, out_operation);
  });
}

auto mln_map_camera_query_take_result(
  mln_operation operation, mln_camera_query_result* out_result
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_camera_query_take_result(operation, out_result);
  });
}

auto mln_map_set_projection_mode(
  mln_map map, const mln_projection_mode* mode, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_set_projection_mode(map, mode, out_command_id);
  });
}

auto mln_map_set_debug_options(
  mln_map map, uint32_t options, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    const auto validation = mln::core::validate_debug_options_input(options);
    if (validation != MLN_STATUS_OK) return validation;
    return mln::core::submit_map_command(
      map,
      [map, options] { return mln::core::map_set_debug_options(map, options); },
      out_command_id
    );
  });
}

auto mln_map_set_rendering_stats_view_enabled(
  mln_map map, bool enabled, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::submit_map_command(
      map,
      [map, enabled] {
        return mln::core::map_set_rendering_stats_view_enabled(map, enabled);
      },
      out_command_id
    );
  });
}

auto mln_map_dump_debug_logs(mln_map map, uint64_t* out_command_id) noexcept
  -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::submit_map_command(
      map, [map] { return mln::core::map_dump_debug_logs(map); }, out_command_id
    );
  });
}

auto mln_map_set_viewport_options(
  mln_map map, const mln_map_viewport_options* options, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    const auto validation = mln::core::validate_viewport_options_input(options);
    if (validation != MLN_STATUS_OK) return validation;
    const auto copied = *options;
    return mln::core::submit_map_command(
      map,
      [map, copied] {
        return mln::core::map_set_viewport_options(map, &copied);
      },
      out_command_id
    );
  });
}

auto mln_map_set_tile_options(
  mln_map map, const mln_map_tile_options* options, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    const auto validation = mln::core::validate_tile_options_input(options);
    if (validation != MLN_STATUS_OK) return validation;
    const auto copied = *options;
    return mln::core::submit_map_command(
      map,
      [map, copied] { return mln::core::map_set_tile_options(map, &copied); },
      out_command_id
    );
  });
}

auto mln_map_pixel_for_lat_lng_start(
  mln_map map, mln_lat_lng coordinate, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_pixel_for_lat_lng_start(
      map, coordinate, out_operation
    );
  });
}

auto mln_map_pixel_for_lat_lng_take_result(
  mln_operation operation, mln_screen_point* out_point
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_pixel_for_lat_lng_take_result(operation, out_point);
  });
}

auto mln_map_lat_lng_for_pixel_start(
  mln_map map, mln_screen_point point, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_lat_lng_for_pixel_start(map, point, out_operation);
  });
}

auto mln_map_lat_lng_for_pixel_take_result(
  mln_operation operation, mln_lat_lng* out_coordinate
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_lat_lng_for_pixel_take_result(
      operation, out_coordinate
    );
  });
}

auto mln_map_pixels_for_lat_lngs_start(
  mln_map map, const mln_lat_lng* coordinates, size_t coordinate_count,
  mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_pixels_for_lat_lngs_start(
      map, coordinates, coordinate_count, out_operation
    );
  });
}

auto mln_map_pixels_for_lat_lngs_take_result(
  mln_operation operation, mln_screen_point* out_points, size_t point_capacity,
  size_t* out_point_count
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_pixels_for_lat_lngs_take_result(
      operation, out_points, point_capacity, out_point_count
    );
  });
}

auto mln_map_lat_lngs_for_pixels_start(
  mln_map map, const mln_screen_point* points, size_t point_count,
  mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_lat_lngs_for_pixels_start(
      map, points, point_count, out_operation
    );
  });
}

auto mln_map_lat_lngs_for_pixels_take_result(
  mln_operation operation, mln_lat_lng* out_coordinates,
  size_t coordinate_capacity, size_t* out_coordinate_count
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_lat_lngs_for_pixels_take_result(
      operation, out_coordinates, coordinate_capacity, out_coordinate_count
    );
  });
}

auto mln_map_camera_for_lat_lng_bounds_start(
  mln_map map, mln_lat_lng_bounds bounds,
  const mln_camera_fit_options* fit_options, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_camera_for_lat_lng_bounds_start(
      map, bounds, fit_options, out_operation
    );
  });
}

auto mln_map_camera_for_lat_lng_bounds_take_result(
  mln_operation operation, mln_camera_options* out_camera
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_camera_for_lat_lng_bounds_take_result(
      operation, out_camera
    );
  });
}

auto mln_map_camera_for_lat_lngs_start(
  mln_map map, const mln_lat_lng* coordinates, size_t coordinate_count,
  const mln_camera_fit_options* fit_options, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_camera_for_lat_lngs_start(
      map, coordinates, coordinate_count, fit_options, out_operation
    );
  });
}

auto mln_map_camera_for_lat_lngs_take_result(
  mln_operation operation, mln_camera_options* out_camera
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_camera_for_lat_lngs_take_result(
      operation, out_camera
    );
  });
}

auto mln_map_camera_for_geometry_start(
  mln_map map, mln_buffer_view geometry,
  const mln_camera_fit_options* fit_options, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_camera_for_geometry_start(
      map, geometry, fit_options, out_operation
    );
  });
}

auto mln_map_camera_for_geometry_take_result(
  mln_operation operation, mln_camera_options* out_camera
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_camera_for_geometry_take_result(
      operation, out_camera
    );
  });
}

auto mln_map_lat_lng_bounds_for_camera_start(
  mln_map map, const mln_camera_options* camera, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_lat_lng_bounds_for_camera_start(
      map, camera, out_operation
    );
  });
}

auto mln_map_lat_lng_bounds_for_camera_take_result(
  mln_operation operation, mln_lat_lng_bounds* out_bounds
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_lat_lng_bounds_for_camera_take_result(
      operation, out_bounds
    );
  });
}

auto mln_map_lat_lng_bounds_for_camera_unwrapped_start(
  mln_map map, const mln_camera_options* camera, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_lat_lng_bounds_for_camera_unwrapped_start(
      map, camera, out_operation
    );
  });
}

auto mln_map_lat_lng_bounds_for_camera_unwrapped_take_result(
  mln_operation operation, mln_lat_lng_bounds* out_bounds
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_lat_lng_bounds_for_camera_unwrapped_take_result(
      operation, out_bounds
    );
  });
}

auto mln_map_set_bounds(
  mln_map map, const mln_bound_options* options, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    const auto validation = mln::core::validate_bound_options_input(options);
    if (validation != MLN_STATUS_OK) return validation;
    const auto copied = *options;
    return mln::core::submit_map_command(
      map, [map, copied] { return mln::core::map_set_bounds(map, &copied); },
      out_command_id
    );
  });
}

auto mln_map_set_free_camera_options(
  mln_map map, const mln_free_camera_options* options, uint64_t* out_command_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    const auto validation =
      mln::core::validate_free_camera_options_input(options);
    if (validation != MLN_STATUS_OK) return validation;
    const auto copied = *options;
    return mln::core::submit_map_command(
      map,
      [map, copied] {
        return mln::core::map_set_free_camera_options(map, &copied);
      },
      out_command_id
    );
  });
}
