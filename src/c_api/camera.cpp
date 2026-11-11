#define MLN_BUILDING_C

#include <cstddef>
#include <cstdint>

#include "c_api/boundary.hpp"
#include "map/map.hpp"
#include "maplibre_native_c.h"

auto mln_map_update_camera(
  mln_map map, const mln_camera_update* update,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::map_update_camera(map, update, completion);
  });
}

auto mln_map_apply_camera_delta(
  mln_map map, const mln_camera_delta* delta, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::map_apply_camera_delta(map, delta, completion);
  });
}

auto mln_map_cancel_camera_transitions(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::map_cancel_camera_transitions(map, completion);
  });
}

auto mln_map_cancel_camera_transitions_with_id(
  mln_map map, uint64_t transition_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::map_cancel_camera_transitions_with_id(
      map, transition_id, completion
    );
  });
}

auto mln_map_get_camera(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::map_camera_query_start(map, completion);
  });
}

auto mln_map_set_projection_mode(
  mln_map map, const mln_projection_mode_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::map_set_projection_mode(map, options, completion);
  });
}

auto mln_map_set_debug_flags(
  mln_map map, uint32_t flags, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    const auto validation = mln::core::validate_debug_flags_input(flags);
    if (validation != MLN_STATUS_OK) return validation;
    return mln::core::submit_map_command(
      map,
      [flags](mln::core::MapObject& live) {
        return mln::core::map_set_debug_flags(live, flags);
      },
      completion
    );
  });
}

auto mln_map_set_globe_free_bearing_enabled(
  mln_map map, bool enabled, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::submit_map_command(
      map,
      [enabled](mln::core::MapObject& live) {
        return mln::core::map_set_globe_free_bearing_enabled(live, enabled);
      },
      completion
    );
  });
}

auto mln_map_set_rendering_stats_view_enabled(
  mln_map map, bool enabled, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::submit_map_command(
      map,
      [enabled](mln::core::MapObject& live) {
        return mln::core::map_set_rendering_stats_view_enabled(live, enabled);
      },
      completion
    );
  });
}

auto mln_map_dump_debug_logs(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::submit_map_command(
      map,
      [](mln::core::MapObject& live) {
        return mln::core::map_dump_debug_logs(live);
      },
      completion
    );
  });
}

auto mln_map_set_viewport_options(
  mln_map map, const mln_map_viewport_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    const auto validation = mln::core::validate_viewport_options_input(options);
    if (validation != MLN_STATUS_OK) return validation;
    const auto copied = *options;
    return mln::core::submit_map_command(
      map,
      [copied](mln::core::MapObject& live) {
        return mln::core::map_set_viewport_options(live, &copied);
      },
      completion
    );
  });
}

auto mln_map_set_tile_options(
  mln_map map, const mln_map_tile_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    const auto validation = mln::core::validate_tile_options_input(options);
    if (validation != MLN_STATUS_OK) return validation;
    const auto copied = *options;
    return mln::core::submit_map_command(
      map,
      [copied](mln::core::MapObject& live) {
        return mln::core::map_set_tile_options(live, &copied);
      },
      completion
    );
  });
}

auto mln_map_camera_for_lat_lng_bounds(
  mln_map map, mln_lat_lng_bounds bounds,
  const mln_camera_fit_options* fit_options, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::map_camera_for_lat_lng_bounds_start(
      map, bounds, fit_options, completion
    );
  });
}

auto mln_map_camera_for_lat_lngs(
  mln_map map, const mln_lat_lng* coordinates, size_t coordinate_count,
  const mln_camera_fit_options* fit_options, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::map_camera_for_lat_lngs_start(
      map, coordinates, coordinate_count, fit_options, completion
    );
  });
}

auto mln_map_camera_for_geometry(
  mln_map map, mln_buffer_view geometry,
  const mln_camera_fit_options* fit_options, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::map_camera_for_geometry_start(
      map, geometry, fit_options, completion
    );
  });
}

auto mln_map_fit_camera_to_lat_lng_bounds(
  mln_map map, mln_lat_lng_bounds bounds, const mln_camera_fit_update* update,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::map_fit_camera_to_lat_lng_bounds(
      map, bounds, update, completion
    );
  });
}

auto mln_map_fit_camera_to_lat_lngs(
  mln_map map, const mln_lat_lng* coordinates, size_t coordinate_count,
  const mln_camera_fit_update* update, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::map_fit_camera_to_lat_lngs(
      map, coordinates, coordinate_count, update, completion
    );
  });
}

auto mln_map_fit_camera_to_geometry(
  mln_map map, mln_buffer_view geometry, const mln_camera_fit_update* update,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::map_fit_camera_to_geometry(
      map, geometry, update, completion
    );
  });
}

auto mln_camera_for_tile(
  mln_canonical_tile_id tile, mln_logical_size logical_size,
  mln_camera_options* out_camera, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::camera_for_tile(tile, logical_size, out_camera);
  });
}

auto mln_map_lat_lng_bounds_for_camera(
  mln_map map, const mln_camera_options* camera,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::map_lat_lng_bounds_for_camera_start(
      map, camera, completion
    );
  });
}

auto mln_map_set_bounds(
  mln_map map, const mln_bound_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    const auto validation = mln::core::validate_bound_options_input(options);
    if (validation != MLN_STATUS_OK) return validation;
    const auto copied = *options;
    return mln::core::submit_map_command(
      map,
      [copied](mln::core::MapObject& live) {
        return mln::core::map_set_bounds(live, &copied);
      },
      completion
    );
  });
}

auto mln_map_set_free_camera_options(
  mln_map map, const mln_free_camera_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    const auto validation =
      mln::core::validate_free_camera_options_input(options);
    if (validation != MLN_STATUS_OK) return validation;
    const auto copied = *options;
    return mln::core::submit_map_command(
      map,
      [copied](mln::core::MapObject& live) {
        return mln::core::map_set_free_camera_options(live, &copied);
      },
      completion
    );
  });
}
