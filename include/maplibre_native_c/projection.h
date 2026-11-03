/**
 * @file maplibre_native_c/projection.h
 * Convert between coordinates, screen points, and projected meters.
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
 * Creates a projection that converts coordinates for one view of a map.
 *
 * Create it from the `view` of an mln_map_snapshot or an
 * mln_render_frame_result. The projection doesn't follow the map, so create a
 * new one after the map's camera or size changes.
 */
MLN_API mln_status mln_map_projection_create(
  const mln_map_view* view,
  mln_map_projection* out_projection MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Releases a projection. */
MLN_API void mln_map_projection_release(
  mln_map_projection projection
) MLN_NOEXCEPT;

/** A projection's camera, as mln_map_projection_get_camera() reports it. */
typedef struct mln_map_projection_camera_result {
  uint32_t size;
  mln_camera camera;
} mln_map_projection_camera_result;

/** Reads the projection's camera. */
MLN_API mln_status mln_map_projection_get_camera(
  mln_map_projection projection,
  mln_map_projection_camera_result* out_result MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Moves the projection's camera, without changing the map.
 *
 * Camera fields that `camera` doesn't set keep their values.
 */
MLN_API mln_status mln_map_projection_set_camera(
  mln_map_projection projection, const mln_camera_options* camera,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Finds the screen point of a coordinate.
 *
 * The point is in the copy of the world nearest the camera's center.
 */
MLN_API mln_status mln_map_projection_pixel_for_lat_lng(
  mln_map_projection projection, mln_lat_lng coordinate,
  mln_screen_point* out_point MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Finds the coordinate at a screen point.
 *
 * The longitude stays between -180 and 180.
 */
MLN_API mln_status mln_map_projection_lat_lng_for_pixel(
  mln_map_projection projection, mln_screen_point point,
  mln_lat_lng* out_coordinate MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Finds the coordinate at a screen point, without wrapping its longitude.
 *
 * The longitude can fall outside -180 to 180, to show which copy of the world
 * the point is in.
 */
MLN_API mln_status mln_map_projection_lat_lng_for_pixel_unwrapped(
  mln_map_projection projection, mln_screen_point point,
  mln_lat_lng* out_coordinate MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Finds the screen points of several coordinates, as
 * mln_map_projection_pixel_for_lat_lng() does.
 *
 * Lifetimes:
 * `out_points` must have room for `coordinate_count` points.
 */
MLN_API mln_status mln_map_projection_pixels_for_lat_lngs(
  mln_map_projection projection,
  const mln_lat_lng* coordinates MLN_BINDING("length=coordinate_count"),
  size_t coordinate_count,
  mln_screen_point* out_points
    MLN_BINDING("direction=out;length=coordinate_count"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Finds the coordinates at several screen points, as
 * mln_map_projection_lat_lng_for_pixel() does.
 *
 * Lifetimes:
 * `out_coordinates` must have room for `point_count` coordinates.
 */
MLN_API mln_status mln_map_projection_lat_lngs_for_pixels(
  mln_map_projection projection,
  const mln_screen_point* points MLN_BINDING("length=point_count"),
  size_t point_count,
  mln_lat_lng* out_coordinates MLN_BINDING("direction=out;length=point_count"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Finds the coordinates at several screen points, as
 * mln_map_projection_lat_lng_for_pixel_unwrapped() does.
 *
 * Lifetimes:
 * `out_coordinates` must have room for `point_count` coordinates.
 */
MLN_API mln_status mln_map_projection_lat_lngs_for_pixels_unwrapped(
  mln_map_projection projection,
  const mln_screen_point* points MLN_BINDING("length=point_count"),
  size_t point_count,
  mln_lat_lng* out_coordinates MLN_BINDING("direction=out;length=point_count"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Computes how many meters one screen pixel covers at `latitude`, at the
 * projection's zoom.
 *
 * The result ignores the camera's pitch.
 */
MLN_API mln_status mln_map_projection_meters_per_pixel_at_latitude(
  mln_map_projection projection, double latitude,
  double* out_meters_per_pixel MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Converts a coordinate to meters in the Web Mercator projection. */
MLN_API mln_status mln_projected_meters_for_lat_lng(
  mln_lat_lng coordinate,
  mln_projected_meters* out_meters MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Converts meters in the Web Mercator projection to a coordinate. */
MLN_API mln_status mln_lat_lng_for_projected_meters(
  mln_projected_meters meters,
  mln_lat_lng* out_coordinate MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_PROJECTION_H
