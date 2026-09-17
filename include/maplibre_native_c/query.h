/**
 * @file maplibre_native_c/query.h
 * Public C API declarations for feature queries.
 */

#ifndef MAPLIBRE_NATIVE_C_QUERY_H
#define MAPLIBRE_NATIVE_C_QUERY_H

#include <stddef.h>
#include <stdint.h>

#include "base.h"
#include "completion.h"
#include "map.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Internal handle for a queried-feature list.
 *
 * No public function accepts or returns one; a query completion borrows the
 * hits directly. The type stays declared here because the render session
 * carries it between the query and its completion.
 */
typedef uint64_t mln_queried_feature_list;

/** Rendered feature query geometry variants. */
typedef enum mln_rendered_query_geometry_type : uint32_t {
  MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT = 1,
  MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX = 2,
  MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING = 3,
} mln_rendered_query_geometry_type;

/**
 * Screen-space box in logical map pixels.
 *
 * Corners may be given in any order and may extend past the viewport. Rendered
 * queries normalize the corners and clip the box to the viewport.
 */
typedef struct mln_screen_box {
  mln_screen_point min;
  mln_screen_point max;
} mln_screen_box;

/** Screen-space line string in logical map pixels. */
typedef struct mln_screen_line_string {
  /** Points. Null only when point_count is 0. */
  const mln_screen_point* points;
  size_t point_count;
} mln_screen_line_string;

/** Rendered feature query geometry descriptor. */
typedef struct mln_rendered_query_geometry {
  uint32_t size;
  /** One of mln_rendered_query_geometry_type. */
  uint32_t type;
  union {
    mln_screen_point point;
    mln_screen_box box;
    mln_screen_line_string line_string;
  } data;
} mln_rendered_query_geometry;

/** Optional fields for mln_rendered_feature_query_options. */
typedef enum mln_rendered_feature_query_option_field : uint32_t {
  MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS = 1U << 0U,
} mln_rendered_feature_query_option_field;

/** Options for rendered feature queries. */
typedef struct mln_rendered_feature_query_options {
  uint32_t size;
  uint32_t fields;
  /** Optional style layer IDs. When absent, all rendered layers are queried. */
  const mln_buffer_view* layer_ids;
  size_t layer_id_count;
  /** Optional UTF-8 MapLibre style-spec filter JSON. Null means no filter. */
  const mln_buffer_view* filter;
} mln_rendered_feature_query_options;

/** Optional fields for mln_source_feature_query_options. */
typedef enum mln_source_feature_query_option_field : uint32_t {
  MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS = 1U << 0U,
} mln_source_feature_query_option_field;

/** Options for source feature queries. */
typedef struct mln_source_feature_query_options {
  uint32_t size;
  uint32_t fields;
  /** Optional source-layer IDs. Required by vector sources; ignored by GeoJSON.
   */
  const mln_buffer_view* source_layer_ids;
  size_t source_layer_id_count;
  /** Optional UTF-8 MapLibre style-spec filter JSON. Null means no filter. */
  const mln_buffer_view* filter;
} mln_source_feature_query_options;

/** Optional fields for mln_queried_feature. */
typedef enum mln_queried_feature_field : uint32_t {
  MLN_QUERIED_FEATURE_SOURCE_ID = 1U << 0U,
  MLN_QUERIED_FEATURE_SOURCE_LAYER_ID = 1U << 1U,
  MLN_QUERIED_FEATURE_STATE = 1U << 2U,
} mln_queried_feature_field;

/**
 * One query hit borrowed for a completion callback.
 *
 * Every view is valid only for that callback; copy what the host keeps.
 * feature is one UTF-8 GeoJSON Feature. source_id, source_layer_id, and state
 * are present when the matching field bit is set. state is a UTF-8 JSON
 * object.
 */
typedef struct mln_queried_feature {
  uint32_t size;
  uint32_t fields;
  mln_buffer_view feature;
  mln_buffer_view source_id;
  mln_buffer_view source_layer_id;
  mln_buffer_view state;
} mln_queried_feature;

/** Returns default rendered feature query options. */
MLN_API mln_rendered_feature_query_options
mln_rendered_feature_query_options_default(void) MLN_NOEXCEPT;

/** Returns default source feature query options. */
MLN_API mln_source_feature_query_options
mln_source_feature_query_options_default(void) MLN_NOEXCEPT;

/** Returns a rendered point query geometry descriptor. */
MLN_API mln_rendered_query_geometry
mln_rendered_query_geometry_point(mln_screen_point point) MLN_NOEXCEPT;

/** Returns a rendered box query geometry descriptor. */
MLN_API mln_rendered_query_geometry
mln_rendered_query_geometry_box(mln_screen_box box) MLN_NOEXCEPT;

/** Returns a rendered line-string query geometry descriptor. */
MLN_API mln_rendered_query_geometry mln_rendered_query_geometry_line_string(
  const mln_screen_point* points, size_t point_count
) MLN_NOEXCEPT;

/**
 * Starts a rendered-feature query against the session's latest driver state.
 *
 * All inputs are copied before return. Core-worker sessions execute on their
 * worker. Caller-driver sessions publish driver work and complete only after
 * the host services it on the graphics thread. The completion borrows an array
 * of mln_queried_feature values (value_count entries), valid only for the
 * callback.
 *
 * Box geometry is normalized and clipped to the viewport, so a box that
 * over-covers the viewport queries everything visible. A box that lies entirely
 * outside the viewport yields an empty result. Point and line-string geometry
 * are queried as given.
 *
 * Returns:
 * - MLN_STATUS_OK when the query is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, geometry is
 *   null, undersized, or names an unknown kind, options is undersized or
 *   carries an invalid field, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the session is not attached.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_TARGET_LOST when the session is abandoned or its target is lost
 *   before the query runs.
 * - MLN_STATUS_NATIVE_ERROR when the query throws on the driver.
 */
MLN_API mln_status mln_render_session_query_rendered_features(
  mln_render_session session, const mln_rendered_query_geometry* geometry,
  const mln_rendered_feature_query_options* options,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts a source-feature query against the session's latest driver state.
 * The completion borrows an array of mln_queried_feature values (value_count
 * entries), valid only for the callback.
 *
 * Returns:
 * - MLN_STATUS_OK when the query is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, source_id is
 *   invalid or empty, options is undersized or carries an invalid field, or
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the session is not attached.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_TARGET_LOST when the session is abandoned or its target is lost
 *   before the query runs.
 * - MLN_STATUS_NATIVE_ERROR when the query throws on the driver.
 */
MLN_API mln_status mln_render_session_query_source_features(
  mln_render_session session, mln_buffer_view source_id,
  const mln_source_feature_query_options* options,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts a feature-extension query against the latest driver state. The
 * completion borrows one mln_buffer_view holding UTF-8 JSON (value_count 1),
 * valid only for the callback.
 *
 * Returns:
 * - MLN_STATUS_OK when the query is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, any of
 *   source_id, feature, extension, or extension_field is invalid or empty,
 *   arguments is invalid, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the session is not attached.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_TARGET_LOST when the session is abandoned or its target is lost
 *   before the query runs.
 * - MLN_STATUS_NATIVE_ERROR when the query throws on the driver.
 */
MLN_API mln_status mln_render_session_query_feature_extensions(
  mln_render_session session, mln_buffer_view source_id,
  mln_buffer_view feature, mln_buffer_view extension,
  mln_buffer_view extension_field, const mln_buffer_view* arguments,
  const mln_completion* completion
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_QUERY_H
