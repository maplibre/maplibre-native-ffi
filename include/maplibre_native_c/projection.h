/**
 * @file maplibre_native_c/projection.h
 * Public C API declarations for projection helpers.
 */

#ifndef MAPLIBRE_NATIVE_C_PROJECTION_H
#define MAPLIBRE_NATIVE_C_PROJECTION_H

#include <stddef.h>
#include <stdint.h>

#include "base.h"
#include "map.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a standalone projection of a view, such as the view of a rendered
 * frame or of a map snapshot.
 *
 * Conversions match the transform that the view describes, whatever the
 * map's later camera, size, or constraints. The projection takes the view's
 * state as it is, with no camera jump, so a world copy, a zoom above the
 * map's maximum, or a pitch above the map's maximum converts as it was drawn.
 * The projection carries no camera constraints: mln_map_projection_set_camera()
 * applies only the view's constrain mode and MapLibre Native's absolute zoom
 * and pitch limits. For a camera that the map's constraints limit, use the
 * map's ordered camera queries, such as mln_map_camera_for_lat_lng_bounds().
 *
 * The caller owns the returned handle. out_projection must point to the null
 * handle. Projection calls are synchronous and serialized with each other.
 * This function needs no map or session and may be called from any thread.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when view is null or undersized, a nested
 *   struct lacks a required field or carries an unknown one, a value is not
 *   finite or out of range, or the extent has a zero width or height or a
 *   scale_factor that is not finite and positive; or when out_projection is
 *   null or does not point to the null handle.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_API mln_status mln_map_projection_create(
  const mln_map_view* view,
  mln_map_projection* out_projection MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Releases a standalone projection.
 *
 * The release retires the handle, waits for projection calls already running on
 * other threads, and destroys the projection before it returns. A later call
 * with the retired handle returns MLN_STATUS_INVALID_STATE. This function
 * may be called from any thread.
 *
 * Returns:
 * - MLN_STATUS_OK when the projection was released by this call.
 * - MLN_STATUS_INVALID_ARGUMENT when projection is an invalid handle.
 * - MLN_STATUS_INVALID_STATE when projection has been released.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_API mln_status mln_map_projection_release(
  mln_map_projection projection, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Copies the projection camera into out_camera.
 *
 * The result observes every earlier projection setter. This function may be
 * called from any thread.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when projection is an invalid handle, or
 *   out_camera is null or undersized.
 * - MLN_STATUS_INVALID_STATE when projection has been released.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
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
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_API mln_status mln_map_projection_set_camera(
  mln_map_projection projection, const mln_camera_options* camera,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Converts a geographic coordinate to a screen point.
 *
 * The output uses logical map pixels with an origin at the top-left of the
 * projection viewport. The coordinate converts in the world copy nearest the
 * camera center's unwrapped longitude. The result observes every earlier
 * projection setter. This function may be called from any thread.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when projection is an invalid handle, out_point
 *   is null, or coordinate is out of range.
 * - MLN_STATUS_INVALID_STATE when projection has been released.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
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
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
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
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
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
 * The scale uses MapLibre Native's zoom and latitude clamps.
 * This function may be called from any thread.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when projection is an invalid handle,
 *   out_meters_per_pixel is null, or latitude is not finite or falls outside
 *   the range from -90 to 90 degrees.
 * - MLN_STATUS_INVALID_STATE when projection has been released.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
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
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
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
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
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
