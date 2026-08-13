/**
 * @file maplibre_native_c/camera.h
 * Public C API declarations for map camera and coordinate conversion.
 */

#ifndef MAPLIBRE_NATIVE_C_CAMERA_H
#define MAPLIBRE_NATIVE_C_CAMERA_H

#include <stddef.h>
#include <stdint.h>

#include "base.h"
#include "map.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns empty camera options initialized for this C API version.
 */
MLN_API mln_camera_options mln_camera_options_default(void) MLN_NOEXCEPT;

/** Returns empty animation options initialized for this C API version. */
MLN_API mln_animation_options mln_animation_options_default(void) MLN_NOEXCEPT;
/** Returns an empty atomic camera update initialized for this API version. */
MLN_API mln_camera_update mln_camera_update_default(void) MLN_NOEXCEPT;

/** Returns empty camera fitting options initialized for this C API version. */
MLN_API mln_camera_fit_options
mln_camera_fit_options_default(void) MLN_NOEXCEPT;

/** Returns empty map bound options initialized for this C API version. */
MLN_API mln_bound_options mln_bound_options_default(void) MLN_NOEXCEPT;

/** Returns empty free camera options initialized for this C API version. */
MLN_API mln_free_camera_options
mln_free_camera_options_default(void) MLN_NOEXCEPT;

/**
 * Returns empty axonometric rendering options initialized for this C API
 * version.
 */
MLN_API mln_projection_mode mln_projection_mode_default(void) MLN_NOEXCEPT;

/** Returns empty viewport options initialized for this C API version. */
MLN_API mln_map_viewport_options
mln_map_viewport_options_default(void) MLN_NOEXCEPT;

/** Returns empty tile tuning options initialized for this C API version. */
MLN_API mln_map_tile_options mln_map_tile_options_default(void) MLN_NOEXCEPT;

/** Submits a debug-overlay command. */
MLN_API mln_status mln_map_set_debug_options(
  mln_map map, uint32_t options, uint64_t* out_command_id
) MLN_NOEXCEPT;
/** Starts an ordered debug-overlay query. */
MLN_API mln_status mln_map_get_debug_options_start(
  mln_map map, mln_operation* out_operation
) MLN_NOEXCEPT;
/** Takes the debug-overlay mask exactly once. */
MLN_API mln_status mln_map_get_debug_options_take_result(
  mln_operation operation, uint32_t* out_options
) MLN_NOEXCEPT;
/** Submits a rendering-stats visibility command. */
MLN_API mln_status mln_map_set_rendering_stats_view_enabled(
  mln_map map, bool enabled, uint64_t* out_command_id
) MLN_NOEXCEPT;
/** Starts an ordered rendering-stats visibility query. */
MLN_API mln_status mln_map_get_rendering_stats_view_enabled_start(
  mln_map map, mln_operation* out_operation
) MLN_NOEXCEPT;
/** Takes the rendering-stats visibility exactly once. */
MLN_API mln_status mln_map_get_rendering_stats_view_enabled_take_result(
  mln_operation operation, bool* out_enabled
) MLN_NOEXCEPT;
/** Starts an ordered fully-loaded query. */
MLN_API mln_status mln_map_is_fully_loaded_start(
  mln_map map, mln_operation* out_operation
) MLN_NOEXCEPT;
/** Takes the fully-loaded result exactly once. */
MLN_API mln_status mln_map_is_fully_loaded_take_result(
  mln_operation operation, bool* out_loaded
) MLN_NOEXCEPT;
/** Submits an ordered debug-log command. */
MLN_API mln_status
mln_map_dump_debug_logs(mln_map map, uint64_t* out_command_id) MLN_NOEXCEPT;
/** Starts an ordered viewport-options query. */
MLN_API mln_status mln_map_get_viewport_options_start(
  mln_map map, mln_operation* out_operation
) MLN_NOEXCEPT;
/** Takes viewport options exactly once. */
MLN_API mln_status mln_map_get_viewport_options_take_result(
  mln_operation operation, mln_map_viewport_options* out_options
) MLN_NOEXCEPT;
/** Submits a copied viewport-options command. */
MLN_API mln_status mln_map_set_viewport_options(
  mln_map map, const mln_map_viewport_options* options, uint64_t* out_command_id
) MLN_NOEXCEPT;
/** Starts an ordered tile-options query. */
MLN_API mln_status mln_map_get_tile_options_start(
  mln_map map, mln_operation* out_operation
) MLN_NOEXCEPT;
/** Takes tile options exactly once. */
MLN_API mln_status mln_map_get_tile_options_take_result(
  mln_operation operation, mln_map_tile_options* out_options
) MLN_NOEXCEPT;
/** Submits a copied tile-options command. */
MLN_API mln_status mln_map_set_tile_options(
  mln_map map, const mln_map_tile_options* options, uint64_t* out_command_id
) MLN_NOEXCEPT;

/**
 * Copies the camera from the latest immutable map snapshot.
 *
 * The returned generation identifies the complete map snapshot that supplied
 * the camera. This function never reads mutable MapLibre state.
 */
MLN_API mln_status mln_map_camera_snapshot_get(
  mln_map map, mln_camera_options* out_camera, uint64_t* out_generation
) MLN_NOEXCEPT;

/**
 * Submits one atomic camera update.
 *
 * The update is copied before return, and out_command_id must point to zero.
 * An accepted command receives a runtime-wide monotonic ID. Its terminal
 * disposition is reported by MLN_RUNTIME_EVENT_COMMAND_FINISHED.
 */
MLN_API mln_status mln_map_update_camera(
  mln_map map, const mln_camera_update* update, uint64_t* out_command_id
) MLN_NOEXCEPT;

/**
 * Starts an ordered camera read.
 *
 * The result observes every command committed before this operation.
 */
MLN_API mln_status mln_map_camera_query_start(
  mln_map map, mln_operation* out_operation
) MLN_NOEXCEPT;

/** Takes the result from a completed ordered camera query exactly once. */
MLN_API mln_status mln_map_camera_query_take_result(
  mln_operation operation, mln_camera_query_result* out_result
) MLN_NOEXCEPT;

/**
 * Starts an ordered query for a camera that fits geographic bounds.
 *
 * Inputs are copied before this function returns. The operation observes every
 * map command accepted before it. Pass null fit_options for default fitting
 * controls.
 */
MLN_API mln_status mln_map_camera_for_lat_lng_bounds_start(
  mln_map map, mln_lat_lng_bounds bounds,
  const mln_camera_fit_options* fit_options, mln_operation* out_operation
) MLN_NOEXCEPT;

/** Takes the fitted camera from a completed operation exactly once. */
MLN_API mln_status mln_map_camera_for_lat_lng_bounds_take_result(
  mln_operation operation, mln_camera_options* out_camera
) MLN_NOEXCEPT;

/**
 * Starts an ordered query for a camera that fits geographic coordinates.
 *
 * The operation owns a copy of coordinates and fit_options.
 */
MLN_API mln_status mln_map_camera_for_lat_lngs_start(
  mln_map map, const mln_lat_lng* coordinates, size_t coordinate_count,
  const mln_camera_fit_options* fit_options, mln_operation* out_operation
) MLN_NOEXCEPT;

/** Takes the fitted camera from a completed operation exactly once. */
MLN_API mln_status mln_map_camera_for_lat_lngs_take_result(
  mln_operation operation, mln_camera_options* out_camera
) MLN_NOEXCEPT;

/**
 * Starts an ordered query for a camera that fits a GeoJSON geometry.
 *
 * The operation owns a copy of the geometry bytes and fit_options.
 */
MLN_API mln_status mln_map_camera_for_geometry_start(
  mln_map map, mln_buffer_view geometry,
  const mln_camera_fit_options* fit_options, mln_operation* out_operation
) MLN_NOEXCEPT;

/** Takes the fitted camera from a completed operation exactly once. */
MLN_API mln_status mln_map_camera_for_geometry_take_result(
  mln_operation operation, mln_camera_options* out_camera
) MLN_NOEXCEPT;

/** Starts an ordered wrapped-bounds query for a copied camera. */
MLN_API mln_status mln_map_lat_lng_bounds_for_camera_start(
  mln_map map, const mln_camera_options* camera, mln_operation* out_operation
) MLN_NOEXCEPT;

/** Takes the wrapped bounds from a completed operation exactly once. */
MLN_API mln_status mln_map_lat_lng_bounds_for_camera_take_result(
  mln_operation operation, mln_lat_lng_bounds* out_bounds
) MLN_NOEXCEPT;

/** Starts an ordered unwrapped-bounds query for a copied camera. */
MLN_API mln_status mln_map_lat_lng_bounds_for_camera_unwrapped_start(
  mln_map map, const mln_camera_options* camera, mln_operation* out_operation
) MLN_NOEXCEPT;

/** Takes the unwrapped bounds from a completed operation exactly once. */
MLN_API mln_status mln_map_lat_lng_bounds_for_camera_unwrapped_take_result(
  mln_operation operation, mln_lat_lng_bounds* out_bounds
) MLN_NOEXCEPT;

/** Starts an ordered camera-constraint query. */
MLN_API mln_status mln_map_get_bounds_start(
  mln_map map, mln_operation* out_operation
) MLN_NOEXCEPT;
/** Takes camera constraints exactly once. */
MLN_API mln_status mln_map_get_bounds_take_result(
  mln_operation operation, mln_bound_options* out_options
) MLN_NOEXCEPT;
/** Submits a copied camera-constraint command. */
MLN_API mln_status mln_map_set_bounds(
  mln_map map, const mln_bound_options* options, uint64_t* out_command_id
) MLN_NOEXCEPT;
/** Starts an ordered free-camera query. */
MLN_API mln_status mln_map_get_free_camera_options_start(
  mln_map map, mln_operation* out_operation
) MLN_NOEXCEPT;
/** Takes free-camera options exactly once. */
MLN_API mln_status mln_map_get_free_camera_options_take_result(
  mln_operation operation, mln_free_camera_options* out_options
) MLN_NOEXCEPT;
/** Submits a copied free-camera command. */
MLN_API mln_status mln_map_set_free_camera_options(
  mln_map map, const mln_free_camera_options* options, uint64_t* out_command_id
) MLN_NOEXCEPT;

/**
 * Submits copied axonometric rendering option fields.
 *
 * Only fields selected by mode->fields affect the map. Unspecified fields keep
 * their current native values. out_command_id must point to zero. On success,
 * it receives the command's monotonic runtime order. The terminal disposition
 * arrives through the runtime event stream.
 */
MLN_API mln_status mln_map_set_projection_mode(
  mln_map map, const mln_projection_mode* mode, uint64_t* out_command_id
) MLN_NOEXCEPT;

/**
 * Starts an ordered conversion from a geographic coordinate to a screen point.
 */
MLN_API mln_status mln_map_pixel_for_lat_lng_start(
  mln_map map, mln_lat_lng coordinate, mln_operation* out_operation
) MLN_NOEXCEPT;

/** Takes the screen point from a completed operation exactly once. */
MLN_API mln_status mln_map_pixel_for_lat_lng_take_result(
  mln_operation operation, mln_screen_point* out_point
) MLN_NOEXCEPT;

/**
 * Starts an ordered conversion from a screen point to a geographic coordinate.
 */
MLN_API mln_status mln_map_lat_lng_for_pixel_start(
  mln_map map, mln_screen_point point, mln_operation* out_operation
) MLN_NOEXCEPT;

/** Takes the coordinate from a completed operation exactly once. */
MLN_API mln_status mln_map_lat_lng_for_pixel_take_result(
  mln_operation operation, mln_lat_lng* out_coordinate
) MLN_NOEXCEPT;

/**
 * Starts an ordered conversion of copied coordinates to screen points.
 */
MLN_API mln_status mln_map_pixels_for_lat_lngs_start(
  mln_map map, const mln_lat_lng* coordinates, size_t coordinate_count,
  mln_operation* out_operation
) MLN_NOEXCEPT;

/**
 * Copies converted points from a completed operation and consumes the result.
 *
 * out_point_count receives the required count before capacity is checked. A
 * capacity error leaves the result available for a retry.
 */
MLN_API mln_status mln_map_pixels_for_lat_lngs_take_result(
  mln_operation operation, mln_screen_point* out_points, size_t point_capacity,
  size_t* out_point_count
) MLN_NOEXCEPT;

/**
 * Starts an ordered conversion of copied screen points to coordinates.
 */
MLN_API mln_status mln_map_lat_lngs_for_pixels_start(
  mln_map map, const mln_screen_point* points, size_t point_count,
  mln_operation* out_operation
) MLN_NOEXCEPT;

/**
 * Copies converted coordinates and consumes the completed operation result.
 *
 * out_coordinate_count receives the required count before capacity is checked.
 * A capacity error leaves the result available for a retry.
 */
MLN_API mln_status mln_map_lat_lngs_for_pixels_take_result(
  mln_operation operation, mln_lat_lng* out_coordinates,
  size_t coordinate_capacity, size_t* out_coordinate_count
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_CAMERA_H
