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
 * The returned operation uses the map runtime's notification source. The
 * operation result is an independent projection handle that copies the map's
 * transform state after every earlier map command. Once creation completes,
 * the projection remains usable after its source map and runtime close. This
 * function may be called from any thread.
 *
 * Every later projection call is synchronous, runs on the calling thread, and
 * is internally serialized. A projection never observes map changes made after
 * its creation.
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
 * Closes a standalone projection.
 *
 * The close retires the handle, waits for projection calls already running on
 * other threads, and destroys the projection before it returns. A later call
 * with the retired handle returns MLN_STATUS_INVALID_ARGUMENT. This function
 * may be called from any thread.
 */
MLN_API mln_status
mln_map_projection_close(mln_map_projection projection) MLN_NOEXCEPT;

/**
 * Copies the projection camera into out_camera.
 *
 * out_camera->size must be at least sizeof(mln_camera_options). The result
 * observes every earlier projection setter. This function may be called from
 * any thread.
 */
MLN_API mln_status mln_map_projection_get_camera(
  mln_map_projection projection, mln_camera_options* out_camera
) MLN_NOEXCEPT;

/**
 * Applies a camera update to a standalone projection.
 *
 * Only fields selected by camera->fields affect the projection. The update is
 * applied before this function returns, so a later read or conversion observes
 * it. The map's camera is unaffected. This function may be called from any
 * thread.
 */
MLN_API mln_status mln_map_projection_set_camera(
  mln_map_projection projection, const mln_camera_options* camera
) MLN_NOEXCEPT;

/**
 * Applies a camera fit for geographic coordinates.
 *
 * The fitted camera is applied before this function returns, so a later read
 * or conversion observes it. This function may be called from any thread.
 */
MLN_API mln_status mln_map_projection_set_visible_coordinates(
  mln_map_projection projection, const mln_lat_lng* coordinates,
  size_t coordinate_count, mln_edge_insets padding
) MLN_NOEXCEPT;

/**
 * Applies a camera fit for GeoJSON Geometry bytes.
 *
 * Empty geometry objects and geometry collections with no coordinates are
 * invalid. The fitted camera is applied before this function returns, so a
 * later read or conversion observes it. This function may be called from any
 * thread.
 */
MLN_API mln_status mln_map_projection_set_visible_geometry(
  mln_map_projection projection, mln_buffer_view geometry,
  mln_edge_insets padding
) MLN_NOEXCEPT;

/**
 * Converts a geographic coordinate to a screen point.
 *
 * The output uses logical map pixels with an origin at the top-left of the
 * projection viewport. The result observes every earlier projection setter.
 * This function may be called from any thread.
 */
MLN_API mln_status mln_map_projection_pixel_for_lat_lng(
  mln_map_projection projection, mln_lat_lng coordinate,
  mln_screen_point* out_point
) MLN_NOEXCEPT;

/**
 * Converts a screen point to a geographic coordinate.
 *
 * The input uses logical map pixels with an origin at the top-left of the
 * projection viewport. The result observes every earlier projection setter.
 * This function may be called from any thread.
 */
MLN_API mln_status mln_map_projection_lat_lng_for_pixel(
  mln_map_projection projection, mln_screen_point point,
  mln_lat_lng* out_coordinate
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
