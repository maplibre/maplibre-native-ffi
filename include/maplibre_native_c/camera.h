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
 * The committed mask is visible through mln_map_get_snapshot as
 * snapshot.debug_options.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, options carries
 *   a bit outside mln_map_debug_option, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when applying the mask throws.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_debug_options(
  mln_map map, uint32_t options MLN_BINDING("enum=mln_map_debug_option"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;
/**
 * Submits a rendering-stats visibility command.
 *
 * The committed value is visible through mln_map_get_snapshot as
 * snapshot.rendering_stats_view_enabled.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when applying the visibility throws.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_rendering_stats_view_enabled(
  mln_map map, bool enabled, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;
/**
 * Submits an ordered debug-log command.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when the dump throws.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_dump_debug_logs(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;
/**
 * Submits a copied viewport-options command.
 *
 * The committed options are visible through mln_map_get_snapshot as
 * snapshot.view.viewport.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, options is null,
 *   undersized, or carries an invalid field, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when applying the options throws.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_viewport_options(
  mln_map map, const mln_map_viewport_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;
/**
 * Submits a copied tile-options command.
 *
 * The committed options are visible through mln_map_get_snapshot as
 * snapshot.tile.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, options is null,
 *   undersized, or carries an invalid field, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when applying the options throws.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_tile_options(
  mln_map map, const mln_map_tile_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Submits one atomic camera update.
 *
 * The update is copied before return. The completion reports its terminal
 * disposition and the snapshot generation that the update published.
 * update->gesture_phase is applied around the camera write; see
 * mln_gesture_phase.
 *
 * Each selected camera field replaces that field's animation. Omitted fields
 * retain their animations and timing. A flight couples center and zoom;
 * replacing either stops both. An anchor couples center with all fields in its
 * update; replacing any of them stops that anchored update. Explicit center
 * takes precedence over anchor. Constraints apply to the combined camera.
 *
 * Ease duration defaults to zero. Fly duration is derived from flight distance
 * and velocity when omitted. The command completion reports application of the
 * update, and update->animation.end_handler reports the end of its
 * transitions, including for a jump.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, update is null
 *   or undersized, update->mode or update->gesture_phase is out of range,
 *   update->camera or update->animation is invalid, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when the camera write throws.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_update_camera(
  mln_map map, const mln_camera_update* update,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Submits one atomic relative camera update.
 *
 * The delta is copied before return. Its components and gesture phase apply in
 * one command: render sessions receive one render update for the whole delta,
 * and the completion reports its terminal disposition and the one snapshot
 * generation that the delta published. The command completion reports
 * application of the delta, and delta->animation.end_handler reports the end of
 * all its transitions once.
 *
 * Validation reads only the fields that delta->fields selects.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, delta is null or
 *   undersized, delta->fields carries an unknown bit, delta->gesture_phase is
 *   out of range, a selected offset, bearing, pitch, or anchor is not finite, a
 *   selected scale is not finite and positive, ANCHOR is selected without
 *   SCALE, BEARING, or PITCH, ANCHOR and OFFSET are selected with a positive
 *   animation duration, delta->animation is invalid, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when the camera write throws.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_apply_camera_delta(
  mln_map map, const mln_camera_delta* delta, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Cancels the camera transitions running when this command commits.
 *
 * Each cancelled command's end handler runs with
 * MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED. Cancelling with no transition
 * running commits and changes nothing.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when the cancellation throws.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_cancel_transitions(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Cancels the camera transitions of the commands whose animation carried
 * transition_id, and leaves every other transition running.
 *
 * The command matches the commands running when it commits whose
 * mln_animation_options selected MLN_ANIMATION_OPTION_TRANSITION_ID with this
 * value. The transitions stop where they stand, and each matched command's
 * end handler runs with MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED. Cancelling an
 * identity that no running command carries commits and changes nothing.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when the cancellation throws.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_cancel_camera_transition(
  mln_map map, uint64_t transition_id, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts an ordered camera read.
 *
 * The query observes every command accepted before it. The completion borrows
 * one mln_camera_query_result.
 *
 * Returns:
 * - MLN_STATUS_OK when the query is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when reading the camera throws.
 */
MLN_BINDING("execution=query;result=mln_camera_query_result")
MLN_API mln_status mln_map_get_camera(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts an ordered query for a camera that fits geographic bounds.
 *
 * Inputs are copied before this function returns. The query observes every
 * map command accepted before it. Pass null fit_options for default fitting
 * controls. The completion borrows one mln_camera_options.
 *
 * Returns:
 * - MLN_STATUS_OK when the query is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, bounds is not a
 *   valid ordered pair of coordinates, or fit_options is undersized or carries
 *   an invalid field, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when the fit throws.
 */
MLN_BINDING("execution=query;result=mln_camera_options")
MLN_API mln_status mln_map_camera_for_lat_lng_bounds(
  mln_map map, mln_lat_lng_bounds bounds,
  const mln_camera_fit_options* fit_options MLN_BINDING("nullable=true"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts an ordered query for a camera that fits geographic coordinates.
 *
 * The query owns a copy of coordinates and fit_options. The completion borrows
 * one mln_camera_options.
 *
 * Returns:
 * - MLN_STATUS_OK when the query is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, coordinates is
 *   null, coordinate_count is zero, a coordinate is out of range, or
 *   fit_options is undersized or carries an invalid field, or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when the fit throws.
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
 * Starts an ordered query for a camera that fits a GeoJSON geometry.
 *
 * The query owns a copy of the geometry bytes and fit_options. The completion
 * borrows one mln_camera_options.
 *
 * Returns:
 * - MLN_STATUS_OK when the query is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, geometry is not
 *   a GeoJSON Geometry, carries no coordinate, or fit_options is undersized or
 *   carries an invalid field, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when the fit throws.
 */
MLN_BINDING("execution=query;result=mln_camera_options")
MLN_API mln_status mln_map_camera_for_geometry(
  mln_map map, mln_buffer_view geometry MLN_BINDING("encoding=json"),
  const mln_camera_fit_options* fit_options MLN_BINDING("nullable=true"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts an ordered wrapped-bounds query for a copied camera.
 *
 * The result is the hull of the top-left and bottom-right screen corners for
 * that camera in the current viewport. When bearing and pitch are zero, the
 * box equals the visible area. Those corners are the northwest and southeast
 * of the viewport. Longitudes stay in -180 to 180. The completion borrows one
 * mln_lat_lng_bounds.
 *
 * Returns:
 * - MLN_STATUS_OK when the query is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, camera is null,
 *   undersized, or carries an invalid field, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when the projection throws.
 */
MLN_BINDING("execution=query;result=mln_lat_lng_bounds")
MLN_API mln_status mln_map_lat_lng_bounds_for_camera(
  mln_map map, const mln_camera_options* camera,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts an ordered unwrapped-bounds query for a copied camera.
 *
 * The result is the axis-aligned hull of all four screen corners and the
 * center, which encompasses the projected viewport. Longitudes unwrap onto
 * the shortest path through the center. A viewport that crosses the
 * antimeridian reports values outside -180 to 180. A center exactly on the
 * antimeridian may resolve to 180 or -180 from floating-point rounding, so its
 * hull may extend past either end of that range. The completion borrows one
 * mln_lat_lng_bounds.
 *
 * Returns:
 * - MLN_STATUS_OK when the query is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, camera is null,
 *   undersized, or carries an invalid field, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when the projection throws.
 */
MLN_BINDING("execution=query;result=mln_lat_lng_bounds")
MLN_API mln_status mln_map_lat_lng_bounds_for_camera_unwrapped(
  mln_map map, const mln_camera_options* camera,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Submits a copied camera-constraint command.
 *
 * The committed constraints are visible through mln_map_get_snapshot as
 * snapshot.bounds.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, options is null,
 *   undersized, or carries an invalid field, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when applying the constraints throws.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_bounds(
  mln_map map, const mln_bound_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Submits a copied free-camera command.
 *
 * The committed options are visible through mln_map_get_snapshot as
 * snapshot.free_camera.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, options is null,
 *   undersized, or carries an invalid field, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when applying the options throws.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_free_camera_options(
  mln_map map, const mln_free_camera_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Submits copied axonometric rendering option fields.
 *
 * Only fields selected by mode->fields affect the map. Unspecified fields keep
 * their current native values. The completion reports the terminal
 * disposition and published snapshot generation.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, mode is null,
 *   undersized, or carries an invalid field, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when applying the mode throws.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_projection_mode(
  mln_map map, const mln_projection_mode* mode,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_CAMERA_H
