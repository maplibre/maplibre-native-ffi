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
#include "runtime/runtime.hpp"

auto mln_camera_options_default(void) noexcept -> mln_camera_options {
  return mln::core::camera_options_default();
}

auto mln_animation_options_default(void) noexcept -> mln_animation_options {
  return mln::core::animation_options_default();
}
auto mln_camera_delta_default(void) noexcept -> mln_camera_delta {
  return mln::core::camera_delta_default();
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
  mln_map map, const mln_camera_update* update, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_update_camera(map, update, completion);
  });
}

auto mln_map_apply_camera_delta(
  mln_map map, const mln_camera_delta* delta, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_apply_camera_delta(map, delta, completion);
  });
}

auto mln_map_camera_query(
  mln_map map, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_camera_query_start(map, completion);
  });
}

auto mln_map_set_projection_mode(
  mln_map map, const mln_projection_mode* mode, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::map_set_projection_mode(map, mode, completion);
  });
}

auto mln_map_set_debug_options(
  mln_map map, uint32_t options, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    const auto validation = mln::core::validate_debug_options_input(options);
    if (validation != MLN_STATUS_OK) return validation;
    return mln::core::submit_map_command(
      map,
      [map, options] { return mln::core::map_set_debug_options(map, options); },
      completion
    );
  });
}

auto mln_map_set_rendering_stats_view_enabled(
  mln_map map, bool enabled, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::submit_map_command(
      map,
      [map, enabled] {
        return mln::core::map_set_rendering_stats_view_enabled(map, enabled);
      },
      completion
    );
  });
}

auto mln_map_dump_debug_logs(
  mln_map map, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::submit_map_command(
      map, [map] { return mln::core::map_dump_debug_logs(map); }, completion
    );
  });
}

auto mln_map_set_viewport_options(
  mln_map map, const mln_map_viewport_options* options,
  const mln_completion* completion
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
      completion
    );
  });
}

auto mln_map_set_tile_options(
  mln_map map, const mln_map_tile_options* options,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    const auto validation = mln::core::validate_tile_options_input(options);
    if (validation != MLN_STATUS_OK) return validation;
    const auto copied = *options;
    return mln::core::submit_map_command(
      map,
      [map, copied] { return mln::core::map_set_tile_options(map, &copied); },
      completion
    );
  });
}

auto mln_map_pixel_for_lat_lng(
  mln_map map, mln_lat_lng coordinate, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_pixel_for_lat_lng_start(map, coordinate, completion);
  });
}

auto mln_map_lat_lng_for_pixel(
  mln_map map, mln_screen_point point, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_lat_lng_for_pixel_start(map, point, completion);
  });
}

auto mln_map_pixels_for_lat_lngs(
  mln_map map, const mln_lat_lng* coordinates, size_t coordinate_count,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_pixels_for_lat_lngs_start(
      map, coordinates, coordinate_count, completion
    );
  });
}

auto mln_map_lat_lngs_for_pixels(
  mln_map map, const mln_screen_point* points, size_t point_count,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_lat_lngs_for_pixels_start(
      map, points, point_count, completion
    );
  });
}

auto mln_map_camera_for_lat_lng_bounds(
  mln_map map, mln_lat_lng_bounds bounds,
  const mln_camera_fit_options* fit_options, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_camera_for_lat_lng_bounds_start(
      map, bounds, fit_options, completion
    );
  });
}

auto mln_map_camera_for_lat_lngs(
  mln_map map, const mln_lat_lng* coordinates, size_t coordinate_count,
  const mln_camera_fit_options* fit_options, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_camera_for_lat_lngs_start(
      map, coordinates, coordinate_count, fit_options, completion
    );
  });
}

auto mln_map_camera_for_geometry(
  mln_map map, mln_buffer_view geometry,
  const mln_camera_fit_options* fit_options, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_camera_for_geometry_start(
      map, geometry, fit_options, completion
    );
  });
}

auto mln_map_lat_lng_bounds_for_camera(
  mln_map map, const mln_camera_options* camera,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_lat_lng_bounds_for_camera_start(
      map, camera, completion
    );
  });
}

auto mln_map_lat_lng_bounds_for_camera_unwrapped(
  mln_map map, const mln_camera_options* camera,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::map_lat_lng_bounds_for_camera_unwrapped_start(
      map, camera, completion
    );
  });
}

auto mln_map_set_bounds(
  mln_map map, const mln_bound_options* options,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    const auto validation = mln::core::validate_bound_options_input(options);
    if (validation != MLN_STATUS_OK) return validation;
    const auto copied = *options;
    return mln::core::submit_map_command(
      map, [map, copied] { return mln::core::map_set_bounds(map, &copied); },
      completion
    );
  });
}

auto mln_map_set_free_camera_options(
  mln_map map, const mln_free_camera_options* options,
  const mln_completion* completion
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
      completion
    );
  });
}
