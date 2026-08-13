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
 * Starts creation of a standalone projection from the map's ordered transform
 * state.
 *
 * The returned operation uses the map runtime's notification source. Creation
 * reserves a child before this function returns, so an accepted operation
 * prevents the map from closing. The operation result is a projection handle
 * that is independent of later map changes. This function may be called from
 * any thread.
 */
MLN_API mln_status mln_map_projection_create_start(
  mln_map map, mln_operation* out_operation
) MLN_NOEXCEPT;

/**
 * Takes the projection handle from a successful creation operation.
 *
 * out_projection must point to the null handle. A failed transfer leaves the
 * result available for another take call. The returned projection may be used
 * from any thread.
 */
MLN_API mln_status mln_map_projection_create_take_result(
  mln_operation operation, mln_map_projection* out_projection
) MLN_NOEXCEPT;

/**
 * Starts closing a standalone projection.
 *
 * Synchronous preflight rejects an invalid or already-closing handle. After
 * acceptance, new projection submissions return an invalid-state status. The
 * operation completes after all accepted projection work and releases the
 * projection's map child reservation. This function may be called from any
 * thread.
 */
MLN_API mln_status mln_map_projection_close_start(
  mln_map_projection projection, mln_operation* out_operation
) MLN_NOEXCEPT;

/**
 * Starts an ordered read of the projection camera.
 *
 * The operation observes every projection command that was accepted before
 * this read. The operation uses the projection runtime's notification source.
 * This function may be called from any thread.
 */
MLN_API mln_status mln_map_projection_get_camera_start(
  mln_map_projection projection, mln_operation* out_operation
) MLN_NOEXCEPT;

/**
 * Takes the camera from a successful camera-read operation.
 *
 * out_camera->size must be at least sizeof(mln_camera_options). A failed
 * transfer leaves the result available for another take call.
 */
MLN_API mln_status mln_map_projection_get_camera_take_result(
  mln_operation operation, mln_camera_options* out_camera
) MLN_NOEXCEPT;

/**
 * Submits a camera update to a standalone projection.
 *
 * Only fields selected by camera->fields affect the projection. The function
 * validates and copies the complete input before it returns. out_command_id
 * must point to zero. On success, it receives the command's monotonic runtime
 * order. A terminal disposition arrives through the runtime event stream. This
 * function may be called from any thread.
 */
MLN_API mln_status mln_map_projection_set_camera(
  mln_map_projection projection, const mln_camera_options* camera,
  uint64_t* out_command_id
) MLN_NOEXCEPT;

/**
 * Submits a camera fit for geographic coordinates.
 *
 * The function validates and copies the coordinates and padding before it
 * returns. out_command_id must point to zero. On success, it receives the
 * command's monotonic runtime order. A terminal disposition arrives through
 * the runtime event stream. This function may be called from any thread.
 */
MLN_API mln_status mln_map_projection_set_visible_coordinates(
  mln_map_projection projection, const mln_lat_lng* coordinates,
  size_t coordinate_count, mln_edge_insets padding, uint64_t* out_command_id
) MLN_NOEXCEPT;

/**
 * Submits a camera fit for GeoJSON Geometry bytes.
 *
 * The function parses and copies the geometry and padding before it returns.
 * Empty geometry objects and geometry collections with no coordinates are
 * invalid. out_command_id must point to zero. On success, it receives the
 * command's monotonic runtime order. A terminal disposition arrives through
 * the runtime event stream. This function may be called from any thread.
 */
MLN_API mln_status mln_map_projection_set_visible_geometry(
  mln_map_projection projection, mln_buffer_view geometry,
  mln_edge_insets padding, uint64_t* out_command_id
) MLN_NOEXCEPT;

/**
 * Starts an ordered conversion from a geographic coordinate to a screen point.
 *
 * The output uses logical map pixels with an origin at the top-left of the
 * projection viewport. The operation observes every projection command that
 * was accepted before this read. This function may be called from any thread.
 */
MLN_API mln_status mln_map_projection_pixel_for_lat_lng_start(
  mln_map_projection projection, mln_lat_lng coordinate,
  mln_operation* out_operation
) MLN_NOEXCEPT;

/**
 * Takes the screen point from a successful conversion operation.
 *
 * A failed transfer leaves the result available for another take call.
 */
MLN_API mln_status mln_map_projection_pixel_for_lat_lng_take_result(
  mln_operation operation, mln_screen_point* out_point
) MLN_NOEXCEPT;

/**
 * Starts an ordered conversion from a screen point to a geographic coordinate.
 *
 * The input uses logical map pixels with an origin at the top-left of the
 * projection viewport. The operation observes every projection command that
 * was accepted before this read. This function may be called from any thread.
 */
MLN_API mln_status mln_map_projection_lat_lng_for_pixel_start(
  mln_map_projection projection, mln_screen_point point,
  mln_operation* out_operation
) MLN_NOEXCEPT;

/**
 * Takes the geographic coordinate from a successful conversion operation.
 *
 * A failed transfer leaves the result available for another take call.
 */
MLN_API mln_status mln_map_projection_lat_lng_for_pixel_take_result(
  mln_operation operation, mln_lat_lng* out_coordinate
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
  mln_lat_lng coordinate, mln_projected_meters* out_meters
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
  mln_projected_meters meters, mln_lat_lng* out_coordinate
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_PROJECTION_H
