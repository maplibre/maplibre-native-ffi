/**
 * @file maplibre_native_c/camera.h
 * Public C API declarations for map camera and coordinate conversion.
 */

#ifndef MAPLIBRE_NATIVE_C_CAMERA_H
#define MAPLIBRE_NATIVE_C_CAMERA_H

#include <stddef.h>
#include <stdint.h>

#include "base.h"
#include "completion.h"
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
/** Returns an empty relative camera update initialized for this API version. */
MLN_API mln_camera_delta mln_camera_delta_default(void) MLN_NOEXCEPT;
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

/**
 * Submits a debug-overlay command.
 *
 * The committed mask is visible through mln_map_snapshot_get as
 * snapshot.debug_options.
 */
MLN_API mln_status mln_map_set_debug_options(
  mln_map map, uint32_t options, const mln_completion* completion
) MLN_NOEXCEPT;
/**
 * Submits a rendering-stats visibility command.
 *
 * The committed value is visible through mln_map_snapshot_get as
 * snapshot.rendering_stats_view_enabled.
 */
MLN_API mln_status mln_map_set_rendering_stats_view_enabled(
  mln_map map, bool enabled, const mln_completion* completion
) MLN_NOEXCEPT;
/** Submits an ordered debug-log command. */
MLN_API mln_status mln_map_dump_debug_logs(
  mln_map map, const mln_completion* completion
) MLN_NOEXCEPT;
/**
 * Submits a copied viewport-options command.
 *
 * The committed options are visible through mln_map_snapshot_get as
 * snapshot.viewport.
 */
MLN_API mln_status mln_map_set_viewport_options(
  mln_map map, const mln_map_viewport_options* options,
  const mln_completion* completion
) MLN_NOEXCEPT;
/**
 * Submits a copied tile-options command.
 *
 * The committed options are visible through mln_map_snapshot_get as
 * snapshot.tile.
 */
MLN_API mln_status mln_map_set_tile_options(
  mln_map map, const mln_map_tile_options* options,
  const mln_completion* completion
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
 * The update is copied before return. The completion reports its terminal
 * disposition and the snapshot generation published by a committed update.
 */
MLN_API mln_status mln_map_update_camera(
  mln_map map, const mln_camera_update* update, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Submits one copied relative camera update.
 *
 * The completion reports its terminal disposition and the snapshot generation
 * published by a committed update.
 */
MLN_API mln_status mln_map_apply_camera_delta(
  mln_map map, const mln_camera_delta* delta, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts an ordered camera read.
 *
 * The completion result observes every command accepted before this query.
 */
MLN_API mln_status mln_map_camera_query(
  mln_map map, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts an ordered query for a camera that fits geographic bounds.
 *
 * Inputs are copied before this function returns. The query observes every
 * map command accepted before it. Pass null fit_options for default fitting
 * controls.
 */
MLN_API mln_status mln_map_camera_for_lat_lng_bounds(
  mln_map map, mln_lat_lng_bounds bounds,
  const mln_camera_fit_options* fit_options, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts an ordered query for a camera that fits geographic coordinates.
 *
 * The query owns a copy of coordinates and fit_options.
 */
MLN_API mln_status mln_map_camera_for_lat_lngs(
  mln_map map, const mln_lat_lng* coordinates, size_t coordinate_count,
  const mln_camera_fit_options* fit_options, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts an ordered query for a camera that fits a GeoJSON geometry.
 *
 * The query owns a copy of the geometry bytes and fit_options.
 */
MLN_API mln_status mln_map_camera_for_geometry(
  mln_map map, mln_buffer_view geometry,
  const mln_camera_fit_options* fit_options, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts an ordered wrapped-bounds query for a copied camera.
 *
 * The result is the hull of the top-left and bottom-right screen corners for
 * that camera in the current viewport. When bearing and pitch are zero, the
 * box equals the visible area. Those corners are the northwest and southeast
 * of the viewport. Longitudes stay in -180 to 180.
 */
MLN_API mln_status mln_map_lat_lng_bounds_for_camera(
  mln_map map, const mln_camera_options* camera,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts an ordered unwrapped-bounds query for a copied camera.
 *
 * The result is the axis-aligned hull of all four screen corners and the
 * center, which encompasses the projected viewport. Longitudes unwrap onto
 * the shortest path through the center. A viewport that crosses the
 * antimeridian reports values outside -180 to 180.
 */
MLN_API mln_status mln_map_lat_lng_bounds_for_camera_unwrapped(
  mln_map map, const mln_camera_options* camera,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Submits a copied camera-constraint command.
 *
 * The committed constraints are visible through mln_map_snapshot_get as
 * snapshot.bounds.
 */
MLN_API mln_status mln_map_set_bounds(
  mln_map map, const mln_bound_options* options,
  const mln_completion* completion
) MLN_NOEXCEPT;
/**
 * Submits a copied free-camera command.
 *
 * The committed options are visible through mln_map_snapshot_get as
 * snapshot.free_camera.
 */
MLN_API mln_status mln_map_set_free_camera_options(
  mln_map map, const mln_free_camera_options* options,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Submits copied axonometric rendering option fields.
 *
 * Only fields selected by mode->fields affect the map. Unspecified fields keep
 * their current native values. The completion reports the terminal
 * disposition and published snapshot generation.
 */
MLN_API mln_status mln_map_set_projection_mode(
  mln_map map, const mln_projection_mode* mode, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts an ordered conversion from a geographic coordinate to a screen point.
 *
 * Each conversion queues one map query. Hot paths such as per-pointer-move
 * conversion use a standalone projection, whose conversions are synchronous.
 */
MLN_API mln_status mln_map_pixel_for_lat_lng(
  mln_map map, mln_lat_lng coordinate, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts an ordered conversion from a screen point to a geographic coordinate.
 *
 * The output longitude is wrapped to -180 to 180. Each conversion queues one
 * map query. Hot paths such as per-pointer-move conversion use a standalone
 * projection, whose conversions are synchronous.
 */
MLN_API mln_status mln_map_lat_lng_for_pixel(
  mln_map map, mln_screen_point point, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts an ordered conversion from a screen point to an unwrapped geographic
 * coordinate.
 *
 * The longitude preserves the visible world copy and may fall outside -180 to
 * 180. The completion borrows one mln_lat_lng.
 */
MLN_API mln_status mln_map_lat_lng_for_pixel_unwrapped(
  mln_map map, mln_screen_point point, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts an ordered conversion of copied coordinates to screen points.
 */
MLN_API mln_status mln_map_pixels_for_lat_lngs(
  mln_map map, const mln_lat_lng* coordinates, size_t coordinate_count,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts an ordered conversion of copied screen points to coordinates.
 *
 * The output longitudes are wrapped to -180 to 180.
 */
MLN_API mln_status mln_map_lat_lngs_for_pixels(
  mln_map map, const mln_screen_point* points, size_t point_count,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts an ordered conversion of copied screen points to unwrapped
 * coordinates.
 *
 * Each longitude preserves the visible world copy and may fall outside -180 to
 * 180. The completion borrows mln_lat_lng[value_count].
 */
MLN_API mln_status mln_map_lat_lngs_for_pixels_unwrapped(
  mln_map map, const mln_screen_point* points, size_t point_count,
  const mln_completion* completion
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_CAMERA_H
