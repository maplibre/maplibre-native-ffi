/**
 * @file maplibre_native_c/projection.h
 * Public C API declarations for projection helpers.
 */

#ifndef MAPLIBRE_NATIVE_C_PROJECTION_H
#define MAPLIBRE_NATIVE_C_PROJECTION_H

#include <stddef.h>
#include <stdint.h>

#include "base.h"
#include "completion.h"
#include "map.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Starts creation of a standalone projection from the map's ordered transform
 * state.
 *
 * The completion borrows an independent projection handle that copies the
 * map's transform state after every earlier map command. A binding takes
 * ownership of that handle before the callback returns. The projection remains
 * usable after its source map and runtime close. This function may be called
 * from any thread.
 *
 * Every later projection call is synchronous, runs on the calling thread, and
 * is internally serialized. A projection never observes map changes made after
 * its creation.
 *
 * Returns:
 * - MLN_STATUS_OK when the creation is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when the projection fails to construct on the map
 *   worker.
 */
MLN_BINDING("execution=lifecycle;result=mln_map_projection")
MLN_API mln_status mln_map_create_projection(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Closes a standalone projection.
 *
 * The close retires the handle, waits for projection calls already running on
 * other threads, and destroys the projection before it returns. A later call
 * with the retired handle returns MLN_STATUS_INVALID_STATE. This function
 * may be called from any thread.
 *
 * Returns:
 * - MLN_STATUS_OK when the projection was closed by this call.
 * - MLN_STATUS_INVALID_ARGUMENT when projection is an invalid handle.
 * - MLN_STATUS_INVALID_STATE when projection has been released.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_projection_close(
  mln_map_projection projection, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Copies the projection camera into out_camera.
 *
 * out_camera->size must be at least sizeof(mln_camera_options). The result
 * observes every earlier projection setter. This function may be called from
 * any thread.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when projection is an invalid handle, or
 *   out_camera is null or undersized.
 * - MLN_STATUS_INVALID_STATE when projection has been released.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_projection_get_camera(
  mln_map_projection projection,
  mln_camera_options* out_camera MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Applies a camera update to a standalone projection.
 *
 * Only fields selected by camera->fields affect the projection. The update is
 * applied before this function returns, so a later read or conversion observes
 * it. The map's camera is unaffected. This function may be called from any
 * thread.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when projection is an invalid handle, or camera
 *   is null, undersized, or carries an invalid field.
 * - MLN_STATUS_INVALID_STATE when projection has been released.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_projection_set_camera(
  mln_map_projection projection, const mln_camera_options* camera,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Applies a camera fit for geographic coordinates.
 *
 * The fitted camera is applied before this function returns, so a later read
 * or conversion observes it. This function may be called from any thread.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when projection is an invalid handle,
 *   coordinates is null, coordinate_count is zero, a coordinate is out of
 *   range, or padding is not finite.
 * - MLN_STATUS_INVALID_STATE when projection has been released.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_projection_set_visible_coordinates(
  mln_map_projection projection,
  const mln_lat_lng* coordinates MLN_BINDING("length=coordinate_count"),
  size_t coordinate_count, mln_edge_insets padding,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Applies a camera fit for GeoJSON Geometry bytes.
 *
 * Empty geometry objects and geometry collections with no coordinates are
 * invalid. The fitted camera is applied before this function returns, so a
 * later read or conversion observes it. This function may be called from any
 * thread.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when projection is an invalid handle, geometry
 *   is not a GeoJSON Geometry or carries no coordinate, or padding is not
 *   finite.
 * - MLN_STATUS_INVALID_STATE when projection has been released.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_projection_set_visible_geometry(
  mln_map_projection projection,
  mln_buffer_view geometry MLN_BINDING("encoding=json"),
  mln_edge_insets padding, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Converts a geographic coordinate to a screen point.
 *
 * The output uses logical map pixels with an origin at the top-left of the
 * projection viewport. The result observes every earlier projection setter.
 * This function may be called from any thread.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when projection is an invalid handle, out_point
 *   is null, or coordinate is out of range.
 * - MLN_STATUS_INVALID_STATE when projection has been released.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_projection_pixel_for_lat_lng(
  mln_map_projection projection, mln_lat_lng coordinate,
  mln_screen_point* out_point MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Converts a screen point to a geographic coordinate.
 *
 * The input uses logical map pixels with an origin at the top-left of the
 * projection viewport. The output longitude is wrapped to -180 to 180. The
 * result observes every earlier projection setter. This function may be called
 * from any thread.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when projection is an invalid handle,
 *   out_coordinate is null, or point is not finite.
 * - MLN_STATUS_INVALID_STATE when projection has been released.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_projection_lat_lng_for_pixel(
  mln_map_projection projection, mln_screen_point point,
  mln_lat_lng* out_coordinate MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Converts a screen point to an unwrapped geographic coordinate.
 *
 * The input uses logical map pixels with an origin at the top-left of the
 * projection viewport. The output longitude preserves the visible world copy
 * and may fall outside -180 to 180. The result observes every earlier
 * projection setter. This function may be called from any thread.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when projection is an invalid handle,
 *   out_coordinate is null, or point contains non-finite values.
 * - MLN_STATUS_INVALID_STATE when projection has been released.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_projection_lat_lng_for_pixel_unwrapped(
  mln_map_projection projection, mln_screen_point point,
  mln_lat_lng* out_coordinate MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reads the ground distance covered by one logical map pixel at a latitude for
 * the helper camera zoom.
 *
 * MapLibre Native computes the scale, including its zoom and latitude clamps.
 * This function may be called from any thread.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when projection is an invalid handle,
 *   out_meters_per_pixel is null, or latitude is not finite or falls outside
 *   the range from -90 to 90 degrees.
 * - MLN_STATUS_INVALID_STATE when projection has been released.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_projection_meters_per_pixel_at_latitude(
  mln_map_projection projection, double latitude,
  double* out_meters_per_pixel MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Converts a geographic coordinate to spherical Mercator projected meters.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when out_meters is null or coordinate contains
 *   invalid latitude or longitude values.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_projected_meters_for_lat_lng(
  mln_lat_lng coordinate,
  mln_projected_meters* out_meters MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Converts spherical Mercator projected meters to a geographic coordinate.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when out_coordinate is null or meters contains
 *   non-finite values.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_lat_lng_for_projected_meters(
  mln_projected_meters meters,
  mln_lat_lng* out_coordinate MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_PROJECTION_H
