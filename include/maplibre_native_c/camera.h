/**
 * @file maplibre_native_c/camera.h
 * Move the camera, and set how the map shows and limits its view.
 */

#ifndef MAPLIBRE_NATIVE_C_CAMERA_H
#define MAPLIBRE_NATIVE_C_CAMERA_H

#include <stddef.h>
#include <stdint.h>

#include "base.h"
#include "completion.h"
#include "map.h"
#include "style.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Draws debugging overlays, such as tile borders, on the map.
 *
 * `flags` replaces the overlays set before.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_debug_flags(
  mln_map map, mln_map_debug_flags flags, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;
/** Shows or hides an overlay of rendering statistics on the map. */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_rendering_stats_view_enabled(
  mln_map map, bool enabled, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;
/** Logs debugging information about the map's style and sources. */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_dump_debug_logs(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;
/**
 * Sets how the map orients and constrains its view.
 *
 * Settings that `options` doesn't set keep their current values.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_viewport_options(
  mln_map map, const mln_map_viewport_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;
/**
 * Sets how the map chooses which tiles to load, and at what detail.
 *
 * Settings that `options` doesn't set keep their current values.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_tile_options(
  mln_map map, const mln_map_tile_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Moves the camera, at once or with an animation.
 *
 * Camera fields that `update` doesn't set keep their values, and the update
 * stops any running animation of the fields it sets. The function finishes
 * when the map applies the update, before an animation ends. To learn when an
 * animation ends, set an mln_camera_transition_handler in its animation
 * options.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_update_camera(
  mln_map map, const mln_camera_update* update,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Moves the camera relative to where it is, as a gesture does.
 *
 * `delta` can pan the map's content by an offset on the screen, scale the zoom,
 * and add to the bearing and pitch. An anchor keeps one screen point in place
 * while the scale, bearing, or pitch changes. An anchor needs one of those
 * changes, and combines with an offset only when the delta has no animation.
 * The function finishes when the map applies the delta, before an animation
 * ends.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_apply_camera_delta(
  mln_map map, const mln_camera_delta* delta, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Stops every running camera animation where it is.
 *
 * The end handler of each stopped animation reports
 * MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_cancel_camera_transitions(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Stops the running camera animations whose animation options set
 * `transition_id`.
 *
 * Other animations keep running. The end handler of each stopped animation
 * reports MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_cancel_camera_transitions_with_id(
  mln_map map, uint64_t transition_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reads the map's camera.
 *
 * The query reads the camera after the map applies the commands sent before
 * it.
 */
MLN_BINDING("execution=query;result=mln_camera_query_result")
MLN_API mln_status mln_map_get_camera(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Computes a camera that fits `bounds` in the map's view.
 *
 * The camera keeps the map's bearing and pitch unless `fit_options` sets them,
 * and its zoom stays within the map's zoom limits.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when the padding in `fit_options` leaves no
 *   room for the area: its left and right add up to at least the map's width,
 *   or its top and bottom to at least the map's height.
 */
MLN_BINDING("execution=query;result=mln_camera_options")
MLN_API mln_status mln_map_camera_for_lat_lng_bounds(
  mln_map map, mln_lat_lng_bounds bounds,
  const mln_camera_fit_options* fit_options MLN_BINDING("nullable=true"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Computes a camera that fits `coordinates` in the map's view.
 *
 * `coordinates` must not be empty. The camera keeps the map's bearing and pitch
 * unless `fit_options` sets them, and its zoom stays within the map's zoom
 * limits.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when the padding in `fit_options` leaves no
 *   room for the area: its left and right add up to at least the map's width,
 *   or its top and bottom to at least the map's height.
 */
MLN_BINDING("execution=query;result=mln_camera_options")
MLN_API mln_status mln_map_camera_for_lat_lngs(
  mln_map map,
  const mln_lat_lng* coordinates MLN_BINDING("length=coordinate_count"),
  size_t coordinate_count,
  const mln_camera_fit_options* fit_options MLN_BINDING("nullable=true"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Computes a camera that fits a GeoJSON geometry in the map's view.
 *
 * `geometry` is a GeoJSON geometry object with at least one coordinate, not a
 * feature. The camera keeps the map's bearing and pitch unless `fit_options`
 * sets them, and its zoom stays within the map's zoom limits.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when the padding in `fit_options` leaves no
 *   room for the area: its left and right add up to at least the map's width,
 *   or its top and bottom to at least the map's height.
 */
MLN_BINDING("execution=query;result=mln_camera_options")
MLN_API mln_status mln_map_camera_for_geometry(
  mln_map map, mln_buffer_view geometry MLN_BINDING("encoding=json"),
  const mln_camera_fit_options* fit_options MLN_BINDING("nullable=true"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Computes the camera that shows exactly one tile on a square map.
 *
 * Jump a map in MLN_MAP_MODE_TILE to the camera to render `tile`, where
 * `logical_size` is the map's size. The camera centers the tile and zooms until
 * the tile fills the map: to zoom z for a 512-pixel map, or z - 1 for a
 * 256-pixel one, plus 0.0001 to keep the neighboring tiles out. It looks
 * straight down with north up and no padding, so the map's earlier camera
 * doesn't change the view. The map keeps the camera within its zoom limits, so
 * for a deep tile, raise the maximum zoom past the camera's with
 * mln_map_set_bounds().
 *
 * Returns:
 * - MLN_STATUS_INVALID_ARGUMENT when `logical_size` isn't square, or is wider
 *   than 512 pixels, the width of a vector tile at its zoom. For denser tiles,
 *   raise the map's scale factor instead.
 * - MLN_STATUS_INVALID_ARGUMENT when `tile.x` or `tile.y` isn't less than 2 to
 *   the power `tile.z`, or when the camera's zoom would fall outside 0 to 25.5,
 *   as for tile 0/0/0 on a 256-pixel map.
 */
MLN_API mln_status mln_camera_for_tile(
  mln_canonical_tile_id tile, mln_logical_size logical_size,
  mln_camera_options* out_camera MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Moves the camera to fit `bounds` in the map's view, at once or with an
 * animation.
 *
 * The map computes the camera when it runs the command, from the camera and
 * size that earlier commands leave, as mln_map_camera_for_lat_lng_bounds()
 * would then. The function finishes when the map applies the change, before an
 * animation ends.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when the padding in `update` leaves no room for
 *   the area: its left and right add up to at least the map's width, or its
 *   top and bottom to at least the map's height. The camera doesn't move.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_fit_camera_to_lat_lng_bounds(
  mln_map map, mln_lat_lng_bounds bounds, const mln_camera_fit_update* update,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Moves the camera to fit `coordinates` in the map's view, at once or with an
 * animation.
 *
 * `coordinates` must not be empty. The map computes the camera when it runs the
 * command, from the camera and size that earlier commands leave, as
 * mln_map_camera_for_lat_lngs() would then. The function finishes when the map
 * applies the change, before an animation ends.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when the padding in `update` leaves no room for
 *   the area: its left and right add up to at least the map's width, or its
 *   top and bottom to at least the map's height. The camera doesn't move.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_fit_camera_to_lat_lngs(
  mln_map map,
  const mln_lat_lng* coordinates MLN_BINDING("length=coordinate_count"),
  size_t coordinate_count, const mln_camera_fit_update* update,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Moves the camera to fit a GeoJSON geometry in the map's view, at once or with
 * an animation.
 *
 * `geometry` is a GeoJSON geometry object with at least one coordinate, not a
 * feature. The map computes the camera when it runs the command, from the
 * camera and size that earlier commands leave, as mln_map_camera_for_geometry()
 * would then. The function finishes when the map applies the change, before an
 * animation ends.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when the padding in `update` leaves no room for
 *   the area: its left and right add up to at least the map's width, or its
 *   top and bottom to at least the map's height. The camera doesn't move.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_fit_camera_to_geometry(
  mln_map map, mln_buffer_view geometry MLN_BINDING("encoding=json"),
  const mln_camera_fit_update* update, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Computes the area that the map would show with `camera`.
 *
 * Camera fields that `camera` doesn't set come from the map's camera. The
 * longitudes can fall outside -180 to 180, so a view across the antimeridian
 * gives one continuous range.
 */
MLN_BINDING("execution=query;result=mln_lat_lng_bounds")
MLN_API mln_status mln_map_lat_lng_bounds_for_camera(
  mln_map map, const mln_camera_options* camera,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Limits the camera's center to an area, and its zoom and pitch to ranges.
 *
 * Limits that `options` doesn't set keep their current values. When the camera
 * is outside the new limits, it moves inside them.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_bounds(
  mln_map map, const mln_bound_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Places the camera at a position and orientation in space.
 *
 * The map adjusts them to a camera that it can show. It stops running camera
 * animations first, and their end handlers report
 * MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_free_camera_options(
  mln_map map, const mln_free_camera_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Sets whether the map draws in perspective or axonometric projection, and how
 * much it skews the view.
 *
 * Settings that `options` doesn't set keep their current values.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_projection_mode(
  mln_map map, const mln_projection_mode_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_CAMERA_H
